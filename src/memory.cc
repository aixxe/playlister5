#include <ranges>
#include <algorithm>
#include <fmt/format.h>
#include <Zydis/Zydis.h>

#include "memory.h"

namespace
{
    struct pattern
    {
        std::vector<std::optional<std::uint8_t>> bytes;
        std::optional<std::size_t> offset;
    };

    auto parse_pattern(const std::string_view input) -> pattern
    {
        auto result = pattern {};

        for (auto&& range: input | std::views::split(' '))
        {
            auto hex = std::string_view { range };

            if (hex.empty())
                continue;

            if (!result.offset && hex.starts_with('[') && hex.ends_with(']'))
            {
                hex = hex.substr(1, hex.size() - 2);
                result.offset = result.bytes.size();
            }

            auto byte = std::optional<std::uint8_t> {};

            if (!hex.starts_with('?'))
                byte = std::stoi(hex.data(), nullptr, 16);

            result.bytes.emplace_back(byte);
        }

        return result;
    }

    auto constexpr byte_equal = [] (auto a, auto b)
        { return !b || a == *b; };

    auto find_generic(auto&& method, auto&& region,
        auto&& pattern, const bool silent) -> std::uint8_t*
    {
        auto const [bytes, offset] = parse_pattern(pattern);
        auto const result = method(region, bytes, byte_equal);

        if (result.empty() && !silent)
            throw std::runtime_error { fmt::format
                ("pattern '{}' not found", pattern) };

        return !result.empty() ? result.data() + offset.value_or(0): nullptr;
    }
}

auto memory::find(std::span<std::uint8_t> region,
    std::string_view pattern, const bool silent) -> std::uint8_t*
{
    return find_generic(std::ranges::search, region, pattern, silent);
}

auto memory::rfind(std::span<std::uint8_t> region,
    std::string_view pattern, const bool silent) -> std::uint8_t*
{
    return find_generic(std::ranges::find_end, region, pattern, silent);
}

auto memory::find_all(std::span<std::uint8_t> region,
    std::string_view pattern) -> std::vector<std::uint8_t*>
{
    auto results = std::vector<std::uint8_t*> {};
    auto const [bytes, offset] = parse_pattern(pattern);

    auto const base = region.data();
    auto const end = region.data() + region.size();

    for (auto pos = base; pos < end; )
    {
        auto sub = std::span { pos, static_cast<std::size_t>(end - pos) };
        auto match = std::ranges::search(sub, bytes, byte_equal);

        if (match.empty())
            break;

        results.push_back(match.data());
        pos = match.data() + 1;
    }

    return results;
}

auto memory::follow(std::uint8_t* ptr, std::int64_t operand) -> std::uint8_t*
{
    auto decoder = ZydisDecoder {};
    auto status = ZydisDecoderInit
        (&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { "failed to initialize decoder" };

    auto instruction = ZydisDecodedInstruction {};
    auto operands = std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> {};

    status = ZydisDecoderDecodeFull
        (&decoder, ptr, 32, &instruction, operands.data());

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { "failed to decode instruction" };

    if (operand == -1)
    {
        operand = 0;

        auto const it = std::ranges::find_if(operands, [] (auto&& op)
            { return op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    (op.mem.base == ZYDIS_REGISTER_RIP ||
                     op.mem.base == ZYDIS_REGISTER_EIP); });

        if (it != operands.end())
            operand = std::distance(operands.begin(), it);
    }

    auto rip = std::bit_cast<ZyanU64>(ptr);
    auto result = ZyanU64 {};

    status = ZydisCalcAbsoluteAddress
        (&instruction, &operands[operand], rip, &result);

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { fmt::format
            ("failed to calculate absolute address at {:#x}", rip) };

    return reinterpret_cast<std::uint8_t*>(result);
}