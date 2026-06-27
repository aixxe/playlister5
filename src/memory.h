#pragma once

#include <vector>
#include <cstdint>

namespace memory
{
    auto find(std::span<std::uint8_t> region,
        std::string_view pattern, bool silent = false) -> std::uint8_t*;

    auto rfind(std::span<std::uint8_t> region,
        std::string_view pattern, bool silent = false) -> std::uint8_t*;

    auto find_all(std::span<std::uint8_t> region,
        std::string_view pattern) -> std::vector<std::uint8_t*>;

    auto follow(std::uint8_t* ptr, std::int64_t operand = -1) -> std::uint8_t*;
}