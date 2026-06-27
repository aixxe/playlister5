#pragma once

#include <array>
#include <string>
#include <vector>

#include "bm2dx.h"

namespace playlister
{
    auto constexpr k_default_voice = "syssd_category_voice_playlist";
    auto constexpr k_default_bar_texture = "so_song_memo";
    auto constexpr k_default_header_texture = "ss_music_memo";
    auto constexpr k_default_subheader_texture = "ss_music_selection_memo";
    auto constexpr k_default_ticker_text = "SELECT FROM PLAYLIST";

    enum badge
    {
        BAR_BADGE_WEEKLY,     // Weekly
        BAR_BADGE_FEATURED,   // Today's Featured Songs
        BAR_BADGE_TOURNAMENT, // Venue Tournament
        BAR_BADGE_KAC,        // KAC
    };

    struct chart
    {
        bool valid;
        std::int32_t music_id;
        bm2dx::bar_style bar_style;
        bm2dx::chart_difficulty difficulty;
    };

    struct playlist
    {
        bool inserted;
        std::string name;
        std::string voice;
        std::string texture_bar;
        std::string texture_header;
        std::string texture_subheader;
        std::string ticker_text;
        std::vector<badge> badges;
        std::vector<chart> charts;
        bm2dx::play_style play_style;
        std::array<bm2dx::clear_type, 2> lamp;
    };

    auto import(const std::filesystem::path& dir) -> std::vector<playlist>;
}
