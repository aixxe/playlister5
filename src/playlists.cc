#include <glaze/yaml.hpp>

#include "avs2.h"
#include "playlists.h"

using namespace avs2;
using namespace bm2dx;
using namespace playlister;

template <>
struct glz::meta<play_style>
{
    using enum play_style;

    auto static constexpr value = enumerate(
        "SP", STYLE_SP,
        "DP", STYLE_DP
    );
};

template <>
struct glz::meta<chart_difficulty>
{
    using enum chart_difficulty;

    auto static constexpr value = enumerate(
        "beginner", DIFFICULTY_BEGINNER,
        "normal", DIFFICULTY_NORMAL,
        "hyper", DIFFICULTY_HYPER,
        "another", DIFFICULTY_ANOTHER,
        "leggendaria", DIFFICULTY_LEGGENDARIA
    );
};

namespace
{
    struct yml_chart
    {
        std::int32_t entry_id;
        std::string bar_style;
        chart_difficulty difficulty;

        struct glaze
        {
            using T = yml_chart;

            auto static constexpr value = glz::object(
                "entry id", &T::entry_id,
                "bar style", &T::bar_style,
                "difficulty", &T::difficulty
            );
        };
    };

    struct yml_playlist
    {
        std::string name;
        std::string voice = k_default_voice;
        std::string bar_texture = k_default_bar_texture;
        std::string header_texture = k_default_header_texture;
        std::string subheader_texture = k_default_subheader_texture;
        std::string ticker_text = k_default_ticker_text;
        std::vector<std::string> badges;
        std::vector<yml_chart> charts;
        play_style play_style;

        struct glaze
        {
            using T = yml_playlist;

            auto static constexpr value = glz::object(
                "name", &T::name,
                "voice", &T::voice,
                "bar texture", &T::bar_texture,
                "header texture", &T::header_texture,
                "subheader texture", &T::subheader_texture,
                "ticker text", &T::ticker_text,
                "badges", &T::badges,
                "charts", &T::charts,
                "play style", &T::play_style
            );
        };
    };

    auto convert(const yml_playlist& yml) -> playlist
    {
        auto result = playlist {
            .inserted          = false,
            .name              = yml.name,
            .voice             = yml.voice,
            .texture_bar       = yml.bar_texture,
            .texture_header    = yml.header_texture,
            .texture_subheader = yml.subheader_texture,
            .ticker_text       = yml.ticker_text,
            .play_style        = yml.play_style,
        };

        result.charts.reserve(yml.charts.size());

        auto insert_badge = [&] (auto&& key, auto&& value)
        {
            if (std::ranges::find(result.badges, value) == result.badges.end())
                result.badges.push_back(value);
            else
                log::warn("duplicate badge '{}' for playlist '{}'", key, yml.name);
        };

        for (auto&& badge: yml.badges)
        {
            if (badge == "kac")
                insert_badge(badge, BAR_BADGE_KAC);
            else if (badge == "weekly")
                insert_badge(badge, BAR_BADGE_WEEKLY);
            else if (badge == "featured")
                insert_badge(badge, BAR_BADGE_FEATURED);
            else if (badge == "tournament")
                insert_badge(badge, BAR_BADGE_TOURNAMENT);
            else
                log::warn("unknown badge '{}' for playlist '{}'", badge, yml.name);
        }

        for (auto const& item: yml.charts)
        {
            if (item.entry_id < 0)
                continue;

            auto bar_style = BAR_STYLE_DEFAULT;

            if (item.bar_style == "secret")
                bar_style = BAR_STYLE_SECRET;
            else if (item.bar_style == "lightning")
                bar_style = BAR_STYLE_LIGHTNING;

            result.charts.push_back({
                .valid      = false,
                .music_id   = item.entry_id,
                .bar_style  = bar_style,
                .difficulty = item.difficulty
            });
        }

        return result;
    }

    auto parse(const std::filesystem::path& path) -> std::vector<yml_playlist>
    {
        auto file = std::ifstream { path };
        auto buffer = std::stringstream {};
        buffer << file.rdbuf();

        auto yaml = buffer.str();
        auto result = std::vector<yml_playlist> {};

        auto const error = glz::read_yaml<glz::opts {
            .error_on_unknown_keys = false,
        }>(result, yaml);

        if (!error)
            return result;

        log::warn("failed to parse file '{}': {}",
            path.filename().string(), glz::format_error(error, yaml));

        return {};
    }
}

auto playlister::import(const std::filesystem::path& dir) -> std::vector<playlist>
{
    if (!std::filesystem::is_directory(dir))
        throw std::runtime_error { fmt::format
            ("directory not found: {}", dir.string()) };

    auto files = std::vector<std::filesystem::path> {};

    for (auto&& entry: std::filesystem::recursive_directory_iterator { dir })
    {
        if (!entry.is_regular_file())
            continue;

        auto const& path = entry.path();

        if (path.extension() == ".yml" || path.extension() == ".yaml")
            files.push_back(path);
    }

    std::ranges::sort(files);

    auto result = std::vector<playlist> {};

    for (auto const& path: files)
    {
        auto const playlists = parse(path);

        if (playlists.empty())
            continue;

        for (auto const& yml: playlists)
            result.push_back(convert(yml));

        log::info("loaded {} ({} playlists)",
            path.filename().string(), playlists.size());
    }

    log::info("total {} playlists loaded from {} files",
        result.size(), files.size());

    return result;
}