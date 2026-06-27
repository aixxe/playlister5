#include "avs2.h"
#include "bm2dx.h"
#include "memory.h"

using namespace avs2;
using namespace bm2dx;

namespace
{
    auto find_forward(auto addr, auto size, auto pattern)
        { return memory::find({ addr, addr + size }, pattern); }

    auto find_backward(auto addr, auto size, auto pattern)
        { return memory::rfind({ addr - size, addr }, pattern); }

    auto backtrack_prologue(auto addr, auto size)
        { return find_backward(addr, size, "CC [48]"); }

    template <typename T = std::uint8_t*>
    auto follow_find(auto region, auto pattern) -> T
        { return reinterpret_cast<T>(memory::follow(memory::find(region, pattern))); }

    auto find_vector_ptr_insert(auto region)
    {
        auto const base = follow_find(region, "E8 ? ? ? ? 90 80 7D ? ? 74 ? 48 8B 5D");

        auto result = find_forward(base, 128, "4C 8D 44 24 ? [E8]");
             result = memory::follow(result);

        return reinterpret_cast<decltype(vector_ptr_insert)>(result);
    }

    auto find_bar_badge_hooks(auto region)
    {
        auto const results = memory::find_all(region,
            "49 69 C1 A8 00 00 00 0F B6 84 38");

        if (results.size() != 4)
            throw std::runtime_error { fmt::format
                ("expected 4 badge scan results, got {}", results.size()) };

        for (auto const match: results)
        {
            auto const result = backtrack_prologue(match, 64);

            switch (match[11] & 0x03)
            {
                case 0: hk_badge_weekly_addr     = result; break;
                case 1: hk_badge_featured_addr   = result; break;
                case 2: hk_badge_tournament_addr = result; break;
                case 3: hk_badge_kac_addr        = result; break;
                default:
                    break;
            }
        }
    }

    auto find_system_sounds(auto region) -> std::unordered_map<std::string, int>
    {
        auto const base = memory::find(region, "48 8D 05 ? ? ? ? 48 03 F8 81 C2");

        auto index = 0x2000;
        auto result = std::unordered_map<std::string, int> {};

        auto const head = reinterpret_cast<system_sound_entry*>(memory::follow(base));
        auto const tail = reinterpret_cast<const std::uint8_t*>(head->description);

        for (auto entry = head; reinterpret_cast<std::uint8_t*>(entry) < tail; ++entry)
        {
            if (entry->description == entry->filename)
                continue;

            result[entry->filename] = index;
            index += 0x50;
        }

        return result;
    }
}

auto bm2dx::init(const std::span<std::uint8_t> region) -> void
{
    vector_ptr_insert = find_vector_ptr_insert(region);
    log::misc("found vector insert function at {}", fmt::ptr(vector_ptr_insert));

    hk_clear_lamp_addr = backtrack_prologue(memory::find(region, "48 6B CA ? 48 03 CE 48 8D 0C 4F"), 128);
    log::misc("found clear lamp getter function at {}", fmt::ptr(hk_clear_lamp_addr));

    hk_bar_text_check_addr = memory::find(region, "0F 87 ? ? ? ? 41 81 C0");
    log::misc("found bar text render check at {}", fmt::ptr(hk_bar_text_check_addr));

    hk_bar_text_render_addr = memory::find(region, "45 8B CD B9 ? ? ? ? [E8] ? ? ? ? 90");
    log::misc("found bar text render call at {}", fmt::ptr(hk_bar_text_render_addr));

    hk_bar_populate_addr = follow_find(region, "E8 ? ? ? ? 48 C7 83 ? ? ? ? ? ? ? ? 48 8B C3");
    log::misc("found group populate function at {}", fmt::ptr(hk_bar_populate_addr));

    hk_get_definition_addr = follow_find(region, "E8 ? ? ? ? 0F B6 70");
    log::misc("found category definition getter at {}", fmt::ptr(hk_get_definition_addr));

    hk_folder_voice_id_addr = backtrack_prologue(memory::find(region, "8B 04 88 48 8B 5C 24"), 128);
    log::misc("found folder voice play function at {}", fmt::ptr(hk_folder_voice_id_addr));

    hk_music_select_init_addr = backtrack_prologue(memory::find(region, "48 83 EC 30 48 8B D9 89 11 E8"), 32);
    log::misc("found music select initialize function at {}", fmt::ptr(hk_music_select_init_addr));

    find_bar_badge_hooks(region);
    log::misc("found bar badge accessor functions at:");
    log::misc("  - weekly     => {}", fmt::ptr(hk_badge_weekly_addr));
    log::misc("  - tournament => {}", fmt::ptr(hk_badge_tournament_addr));
    log::misc("  - kac        => {}", fmt::ptr(hk_badge_kac_addr));
    log::misc("  - featured   => {}", fmt::ptr(hk_badge_featured_addr));

    init_category_bar = follow_find<decltype(init_category_bar)>(region, "E8 ? ? ? ? FF C3 48 81 C7 ? ? ? ? 81 FB");
    log::misc("found category initialize function at {}", fmt::ptr(init_category_bar));

    get_music_entry = follow_find<decltype(get_music_entry)>(region, "E8 ? ? ? ? 0F BF B0");
    log::misc("found music entry getter function at {}", fmt::ptr(get_music_entry));

    bar_insert_chart = follow_find<decltype(bar_insert_chart)>(region, "E8 ? ? ? ? 84 C0 75 ? FF C3 83 FB ? 7D");
    log::misc("found bar chart insertion function at {}", fmt::ptr(bar_insert_chart));

    get_score_data = follow_find<decltype(get_score_data)>(region, "E8 ? ? ? ? 85 C0 75 ? 8B 7C 24");
    log::misc("found score data getter function at {}", fmt::ptr(get_score_data));

    system_sounds = find_system_sounds(region);
    log::misc("parsed {} system sounds from table", system_sounds.size());
}
