#pragma once

#include <span>
#include <cstdint>
#include <unordered_map>

namespace bm2dx
{
    enum play_style
    {
        STYLE_SP = 0,
        STYLE_DP = 1,
    };

    enum category_group_type
    {
        GROUP_RECOMMEND   = 0,  // RecommendCategoryGroup
        GROUP_LEVEL       = 1,  // LevelCategoryGroup
        GROUP_VERSION     = 2,  // VersionCategoryGroup
        GROUP_MUSIC_NAME  = 3,  // MusicNameCategoryGroup
        GROUP_NOTES_RADAR = 4,  // NotesRadarCategoryGroup
        GROUP_CUSTOM      = 5,  // CustomCategoryGroup
        GROUP_ALL         = 6,  // AllCategoryGroup
        GROUP_DJ_DATA     = 8,  // DJDataCategoryGroup
        GROUP_RIVAL       = 9,  // RivalCategoryGroup
        GROUP_EVENT       = 10, // EventCategoryGroup
        GROUP_TRAINING    = 11, // TrainingCategoryGroup
    };

    enum chart_difficulty
    {
        DIFFICULTY_BEGINNER    = 0,
        DIFFICULTY_NORMAL      = 1,
        DIFFICULTY_HYPER       = 2,
        DIFFICULTY_ANOTHER     = 3,
        DIFFICULTY_LEGGENDARIA = 4,
    };

    enum clear_type
    {
        CLEAR_NO_PLAY    = 0,
        CLEAR_FAILED     = 1,
        CLEAR_ASSIST     = 2,
        CLEAR_EASY       = 3,
        CLEAR_NORMAL     = 4,
        CLEAR_HARD       = 5,
        CLEAR_EX_HARD    = 6,
        CLEAR_FULL_COMBO = 7,
    };

    enum bar_entry_type
    {
        BAR_TYPE_HEADER  = 0,
        BAR_TYPE_CHART   = 1,
        BAR_TYPE_RANDOM  = 2,
        BAR_TYPE_VIRTUAL = 3,
    };

    enum bar_style
    {
        BAR_STYLE_DEFAULT,
        BAR_STYLE_SECRET,
        BAR_STYLE_LIGHTNING,
    };

    auto inline (*vector_ptr_insert) (void*, void*, void**) -> void* {};

    template <typename T>
    struct vector
    {
        [[nodiscard]] auto operator[] (std::size_t index) noexcept -> T&
            { return data[index]; }

        [[nodiscard]] auto operator[] (std::size_t index) const noexcept -> const T&
            { return data[index]; }

        auto push_back(T&& ptr) -> T*
        {
            static_assert(std::is_pointer_v<T>, "pointer types only");

            if (end >= capacity)
                return static_cast<T*>(vector_ptr_insert
                    (this, end, reinterpret_cast<void**>(&ptr)));

            *end = ptr;
            return end++;
        }

        /* 0x0000 */ T* data;
        /* 0x0008 */ T* end;
        /* 0x0010 */ T* capacity;
    }; static_assert(sizeof(vector<void*>) == 0x18);

    struct small_string
    {
        /* 0x0000 */ char buffer[16];
        /* 0x0010 */ std::size_t size;
        /* 0x0018 */ std::size_t capacity;
    }; static_assert(sizeof(small_string) == 0x20);

    struct CCategoryGameData;
    struct CCustomizeGameData;

    struct system_sound_entry
    {
        /* 0x0000 */ const char* description;
        /* 0x0008 */ const char* filename;
        /* 0x0010 */ std::uint8_t pad_0010[0x278];
    }; static_assert(sizeof(system_sound_entry) == 0x288);

    struct category_definition
    {
        /* 0x0000 */ std::uint8_t pad_0000[8];
        /* 0x0008 */ std::uint8_t difficulty_switchable;
        /* 0x0009 */ std::uint8_t difficulty_locked;
        /* 0x000A */ std::uint8_t pad_000A[6];
        /* 0x0010 */ const char* texture_hover_header;
        /* 0x0018 */ const char* texture_hover_subheader;
        /* 0x0020 */ const char* texture_bar_text;
        /* 0x0028 */ const char* texture_bar_bg;
        /* 0x0030 */ const char* ticker_text;
        /* 0x0038 */ std::int32_t capacity;
        /* 0x003C */ std::uint8_t pad_003C[4];
    }; static_assert(sizeof(category_definition) == 0x40);

    struct header_bar
    {
        /* 0x0000 */ std::int32_t id;
        /* 0x0004 */ std::int32_t active_difficulty;
        /* 0x0008 */ std::uint8_t pad_0008[8];
        /* 0x0010 */ small_string string;
    }; static_assert(sizeof(header_bar) == 0x30);

    struct random_bar
    {
        /* 0x0000 */ std::int32_t category_id;
        /* 0x0004 */ std::uint8_t pad_0004[12];
    }; static_assert(sizeof(random_bar) == 0x10);

    struct chart_bar
    {
        /* 0x0000 */ void* music_entry;
        /* 0x0008 */ std::int32_t p1_difficulty;
        /* 0x000C */ std::int32_t p2_difficulty;
        /* 0x0010 */ std::int32_t p1_original_difficulty;
        /* 0x0014 */ std::int32_t p2_original_difficulty;
        /* 0x0018 */ std::int32_t play_style;
        /* 0x001C */ std::int32_t category_id;
        /* 0x0020 */ std::int32_t bar_style[5];
        /* 0x0034 */ std::uint8_t pad_0034[12];
    }; static_assert(sizeof(chart_bar) == 0x40);

    struct bar_entry
    {
        /* 0x0000 */ bar_entry_type type;
        /* 0x0004 */ std::uint32_t flags;
        /* 0x0008 */ void* data;
    }; static_assert(sizeof(bar_entry) == 0x10);

    struct category
    {
        /* 0x0000 */ std::int32_t id;
        /* 0x0004 */ std::uint8_t is_open;
        /* 0x0005 */ std::uint8_t pad_0005[3];
        /* 0x0008 */ std::int32_t bar_count;
        /* 0x000C */ std::int32_t chart_count;
        /* 0x0010 */ std::int32_t insert_count;
        /* 0x0014 */ std::int32_t active_bar_index;
        /* 0x0018 */ std::uint8_t pad_0018[8];
        /* 0x0020 */ vector<bar_entry> bar_list;
        /* 0x0038 */ header_bar header_bar;
        /* 0x0068 */ random_bar random_bar;
        /* 0x0078 */ vector<chart_bar> chart_list;
        /* 0x0090 */ std::uint8_t pad_0090[64];
    }; static_assert(sizeof(category) == 0xD0);

    struct category_group
    {
        /* 0x0000 */ void* vtable;
        /* 0x0008 */ std::int32_t group_id;
        /* 0x000C */ std::uint8_t pad_000C[108];
        /* 0x0078 */ vector<category*> sp_bars;
        /* 0x0090 */ vector<category*> dp_bars;
    }; static_assert(sizeof(category_group) == 0xA8);

    inline std::uint8_t* hk_clear_lamp_addr        = {};
    inline std::uint8_t* hk_bar_text_check_addr    = {};
    inline std::uint8_t* hk_bar_text_render_addr   = {};
    inline std::uint8_t* hk_bar_populate_addr      = {};
    inline std::uint8_t* hk_get_definition_addr    = {};
    inline std::uint8_t* hk_folder_voice_id_addr   = {};
    inline std::uint8_t* hk_music_select_init_addr = {};

    inline std::uint8_t* hk_badge_weekly_addr      = {};
    inline std::uint8_t* hk_badge_featured_addr    = {};
    inline std::uint8_t* hk_badge_tournament_addr  = {};
    inline std::uint8_t* hk_badge_kac_addr         = {};

    auto inline (*init_category_bar) (void*, int) -> bool {};
    auto inline (*get_music_entry) (int) -> void* {};
    auto inline (*bar_insert_chart) (const void* [5], int, int, int) -> bool {};
    auto inline (*get_score_data) (int, int, int, int, int*, int*, clear_type*) -> int {};

    auto inline system_sounds = std::unordered_map<std::string, int> {};

    auto init(std::span<std::uint8_t> region) -> void;
}