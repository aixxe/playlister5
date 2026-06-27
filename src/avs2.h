#pragma once

#include <fmt/format.h>

namespace avs2
{
    extern "C"
    {
        auto log_body_warning(const char* module, const char* fmt, ...) -> void;
        auto log_body_info(const char* module, const char* fmt, ...) -> void;
        auto log_body_misc(const char* module, const char* fmt, ...) -> void;
    }

    namespace log
    {
        auto constexpr module = "playlister";

        template <typename... Args>
        auto constexpr warn(fmt::format_string<Args...> fmt, Args&&... args)
        {
            log_body_warning(module, "%s", fmt::format(fmt,
                std::forward<Args>(args)...).c_str());
        }

        template <typename... Args>
        auto constexpr info(fmt::format_string<Args...> fmt, Args&&... args)
        {
            log_body_info(module, "%s", fmt::format(fmt,
                std::forward<Args>(args)...).c_str());
        }

        template <typename... Args>
        auto constexpr misc(fmt::format_string<Args...> fmt, Args&&... args)
        {
            log_body_misc(module, "%s", fmt::format(fmt,
                std::forward<Args>(args)...).c_str());
        }
    }
}