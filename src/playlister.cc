#include <safetyhook.hpp>

#include "avs2.h"
#include "bm2dx.h"
#include "memory.h"
#include "modules.h"
#include "playlists.h"

using namespace avs2;
using namespace bm2dx;
using namespace playlister;

auto constexpr k_base_playlist_id = 1000;

auto static g_playlists = std::vector<playlist> {};
auto static g_categories = std::vector<category> {};
auto static g_definitions = std::vector<category_definition> {};

auto static hk_clear_lamp = safetyhook::InlineHook {};
auto static hk_bar_text_check = safetyhook::MidHook {};
auto static hk_bar_text_render = safetyhook::MidHook {};
auto static hk_bar_populate = safetyhook::InlineHook {};
auto static hk_get_definition = safetyhook::InlineHook {};
auto static hk_folder_voice_id = safetyhook::InlineHook {};
auto static hk_music_select_init = safetyhook::InlineHook {};

auto static hk_badge_weekly = safetyhook::InlineHook {};
auto static hk_badge_featured = safetyhook::InlineHook {};
auto static hk_badge_tournament = safetyhook::InlineHook {};
auto static hk_badge_kac = safetyhook::InlineHook {};

auto static is_custom_category(auto category) -> bool
    { return category >= k_base_playlist_id; }

auto static get_custom_index(auto category) -> int
    { return category - k_base_playlist_id; }

auto static get_playlist(auto category) -> playlist&
    { return g_playlists[get_custom_index(category)]; }

/**
 * Create new `category_definition` instances for each playlist
 */
auto static init_custom_category_data()
{
    for (auto i = 0; i < g_playlists.size(); i++)
    {
        auto const& playlist = g_playlists[i];
        auto const category = &g_definitions[i];
        auto const index = k_base_playlist_id + i;

        category->difficulty_switchable   = true;
        category->difficulty_locked       = true;
        category->texture_hover_header    = playlist.texture_header.c_str();
        category->texture_hover_subheader = playlist.texture_subheader.c_str();
        category->texture_bar_bg          = playlist.texture_bar.c_str();
        category->ticker_text             = playlist.ticker_text.c_str();
        category->capacity                = static_cast<std::int32_t>(playlist.charts.size());

        log::misc("allocated category #{} '{}' at {}",
            index, playlist.name, fmt::ptr(category));
    }
}

/**
 * Difficulty remapper for `get_score_data` - SP uses 0..4; DP uses 5..9
 */
auto constexpr remap_difficulty = [] (auto play_style, auto difficulty)
    { return difficulty + (play_style == STYLE_DP ? 5: 0); };

/**
 * Calculate clear lamps for custom playlists
 */
auto static calculate_clear_lamps() -> void
{
    for (auto& playlist: g_playlists)
        playlist.lamp = { CLEAR_FULL_COMBO, CLEAR_FULL_COMBO };

    for (auto& playlist: g_playlists)
    {
        auto const play_style = playlist.play_style;

        for (auto const& chart: playlist.charts)
        {
            if (!chart.valid)
                continue;

            for (auto player = 0; player < 2; player++)
            {
                auto lamp = CLEAR_NO_PLAY;
                auto const result = get_score_data(player, play_style, chart.music_id,
                    remap_difficulty(play_style, chart.difficulty), nullptr, nullptr, &lamp);

                if (result != 0)
                    lamp = CLEAR_NO_PLAY;

                if (lamp < playlist.lamp[player])
                    playlist.lamp[player] = lamp;
            }
        }
    }
}

/**
 * Populate a category with charts from the corresponding playlist
 */
auto static insert_charts(const int id, const int play_style) -> bool
{
    auto& playlist = get_playlist(id);

    if (playlist.inserted)
        return true;

    auto const category = &g_categories[get_custom_index(id)];

    log::misc("inserting {} {} charts into category #{} '{}' at {}",
        playlist.charts.size(), play_style == STYLE_SP ? "sp": "dp",
        id, playlist.name, fmt::ptr(category));

    for (auto i = category->insert_count; i < playlist.charts.size(); i++)
    {
        auto& chart = playlist.charts[i];
        auto const music = get_music_entry(chart.music_id);

        if (!music)
        {
            log::warn("failed to lookup music id {}, skipping entry #{} in '{}'",
                chart.music_id, i, playlist.name);

            continue;
        }

        auto constexpr flag = 0;
        auto params = std::to_array<const void*>
            ({ &music, &category, &chart.difficulty, &flag, &flag });

        auto const result = bar_insert_chart(params.data(),
            chart.difficulty, play_style, id);

        if (!result)
        {
            log::warn("failed to insert chart for music id {}, skipping entry #{} in '{}'",
                chart.music_id, i, playlist.name);

            continue;
        }

        chart.valid = true;

        category->chart_list[i].bar_style[chart.difficulty] = chart.bar_style;
        category->chart_count++;
    }

    playlist.inserted = true;

    log::misc("successfully inserted {} of {} charts into category #{}",
        category->chart_count, playlist.charts.size(), id);

    auto bars = 0;

    category->bar_list[bars++] = { .type = BAR_TYPE_HEADER, .data = &category->header_bar };

    if (category->chart_count > 1)
        category->bar_list[bars++] = { .type = BAR_TYPE_RANDOM, .data = &category->random_bar };

    for (auto i = 0; i < category->chart_count; i++)
        category->bar_list[bars++] = { .type = BAR_TYPE_CHART, .data = &category->chart_list[i] };

    category->bar_count = bars;

    return true;
}

/**
 * Return calculated clear lamp data for custom categories
 */
auto static hk_fn_clear_lamp(CCategoryGameData* base, int category, int difficulty, int player) -> int
{
    if (!is_custom_category(category))
        return hk_clear_lamp.call<int>(base, category, difficulty, player);

    if (player < 0 || player > 1)
        return 0;

    return static_cast<int>(get_playlist(category).lamp[player]);
}

/**
 * Minimal state between the following two mid-function hooks
 */
auto thread_local bar_text_id = std::optional<std::uint32_t> {};

/**
 * Intercept draw text check for each category bar
 */
auto static hk_fn_bar_text_check(SafetyHookContext& ctx) -> void
{
    bar_text_id = static_cast<std::uint32_t>(ctx.r8);

    if (!is_custom_category(*bar_text_id))
        return;

    auto const& texture = get_playlist(*bar_text_id).texture_bar;

    if (!texture.empty() && texture != k_default_bar_texture)
        return;

    ctx.rflags |= 0x40;
}

/**
 * Perform the actual text rendering
 */
auto static hk_fn_bar_text_render(SafetyHookContext& ctx) -> void
{
    if (!bar_text_id || !is_custom_category(*bar_text_id))
        return;

    ctx.rcx = 33;  // Use a slightly better font
    ctx.rdx -= 10; // Shift text to the left slightly

    // Replace the text pointer with our own
    *reinterpret_cast<const char**>(ctx.rsp + 0x28) =
        get_playlist(*bar_text_id).name.c_str();

    bar_text_id.reset();
}

/**
 * Redirect definition lookups within our custom range to the right place
 */
auto static hk_fn_get_definition(CCategoryGameData* base, const int id) -> category_definition*
{
    if (is_custom_category(id))
        return &g_definitions[get_custom_index(id)];

    return hk_get_definition.call<category_definition*>(base, id);
}

/**
 * Set up our category bars after the game has populated the stock stuff
 */
auto static hk_fn_bar_populate(category_group* group) -> void*
{
    auto const result = hk_bar_populate.call<void*>(group);

    if (group->group_id != GROUP_CUSTOM && group->group_id != GROUP_ALL)
        return result;

    for (auto i = 0; i < g_playlists.size(); i++)
    {
        auto const play_style = g_playlists[i].play_style;
        auto& destination = play_style == STYLE_SP ?
            group->sp_bars: group->dp_bars;

        insert_charts(k_base_playlist_id + i, play_style);

        if (g_categories[i].chart_count == 0)
            continue;

        destination.push_back(&g_categories[i]);
    }

    calculate_clear_lamps();

    return result;
}

/**
 * Play a custom system sound when opening our categories
 */
auto static hk_fn_folder_voice_id(CCustomizeGameData* base, int category) -> int
{
    auto const result = hk_folder_voice_id.call<int>(base, category);

    if (!is_custom_category(category))
        return result;

    auto const& playlist = get_playlist(category);

    if (system_sounds.contains(playlist.voice))
        return system_sounds[playlist.voice];

    return result;
}

/**
 * Display various badges on our custom categories
 */
template <auto& hook, auto badge>
auto static hook_get_badge(CCategoryGameData* base, int category) -> int
{
    if (!is_custom_category(category))
        return hook.template call<int>(base, category);

    return std::ranges::contains(get_playlist(category).badges, badge);
}

/**
 * Clean up any stale state upon entering music select
 */
auto static hk_fn_music_select_init(void* a1, int a2) -> void*
{
    for (auto i = 0; i < g_categories.size(); i++)
    {
        init_category_bar(&g_categories[i], k_base_playlist_id + i);

        g_playlists[i].inserted = false;
        g_categories[i].chart_count = 0;
    }

    return hk_music_select_init.call<void*>(a1, a2);
}

/**
 * Get the party started
 */
auto static init(std::uint8_t* module) -> void
{
    auto const start = std::chrono::steady_clock::now();

    log::info("playlister v{} ({}@{}) loaded at {:#x}",
        META_PROJECT_VERSION, META_GIT_BRANCH, META_GIT_COMMIT_SHORT,
        std::bit_cast<std::uintptr_t>(module));

    log::info("built {} {} with {} {}",
        __DATE__, __TIME__, META_COMPILER_ID, META_COMPILER_VERSION);

    auto buffer = std::string(MAX_PATH, '\0');
    auto const length = GetModuleFileNameA(reinterpret_cast<HMODULE>(module),
        buffer.data(), buffer.size());

    if (length == 0 || length == buffer.size())
        throw std::runtime_error { "failed to get module filename" };

    buffer.resize(length);

    auto const cwd = std::filesystem::path { buffer }.remove_filename();

    log::info("running from directory '{}'", cwd.string());

    auto const list = modules::list();
    auto const bm2dx = std::ranges::find_if(list, [] (auto&& entry)
        { return modules::has_export(entry.base, "dll_entry_main"); });

    if (bm2dx == list.end())
        throw std::runtime_error { "game library not found" };

    log::info("detected game library '{}' at {}",
        bm2dx->path.filename().string(), fmt::ptr(bm2dx->base));

    g_playlists = import(cwd / "playlister" / "playlists");

    if (g_playlists.empty())
        throw std::runtime_error { "no playlists found" };

    bm2dx::init(bm2dx->region());

    g_categories.resize(g_playlists.size());
    g_definitions.resize(g_playlists.size());

    hk_clear_lamp = safetyhook::create_inline(hk_clear_lamp_addr, &hk_fn_clear_lamp);
    hk_bar_text_check = safetyhook::create_mid(hk_bar_text_check_addr, hk_fn_bar_text_check);
    hk_bar_text_render = safetyhook::create_mid(hk_bar_text_render_addr, hk_fn_bar_text_render);
    hk_bar_populate = safetyhook::create_inline(hk_bar_populate_addr, &hk_fn_bar_populate);
    hk_get_definition = safetyhook::create_inline(hk_get_definition_addr, &hk_fn_get_definition);
    hk_folder_voice_id = safetyhook::create_inline(hk_folder_voice_id_addr, &hk_fn_folder_voice_id);
    hk_music_select_init = safetyhook::create_inline(hk_music_select_init_addr, &hk_fn_music_select_init);

    hk_badge_weekly = safetyhook::create_inline(hk_badge_weekly_addr, &hook_get_badge<hk_badge_weekly, BAR_BADGE_WEEKLY>);
    hk_badge_featured = safetyhook::create_inline(hk_badge_featured_addr, &hook_get_badge<hk_badge_featured, BAR_BADGE_FEATURED>);
    hk_badge_tournament = safetyhook::create_inline(hk_badge_tournament_addr, &hook_get_badge<hk_badge_tournament, BAR_BADGE_TOURNAMENT>);
    hk_badge_kac = safetyhook::create_inline(hk_badge_kac_addr, &hook_get_badge<hk_badge_kac, BAR_BADGE_KAC>);

    init_custom_category_data();

    auto const elapsed = std::chrono::steady_clock::now() - start;

    log::info("initialized successfully in {:.2f} seconds",
        std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count());
}

auto DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    DisableThreadLibraryCalls(module);

    try
        { ::init(reinterpret_cast<std::uint8_t*>(module)); }
    catch (std::exception const& e)
    {
        log::warn("initialization failed: {}", e.what());
        return FALSE;
    }

    return TRUE;
}