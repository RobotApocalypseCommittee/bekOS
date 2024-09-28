/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_DEBUG_H
#define BEKOS_DEBUG_H

#define KERNEL_DEBUG

#ifdef KERNEL_DEBUG

#include "bek/format.h"

extern bek::OutputStream* debug_stream;

template <unsigned N>
struct FixedString {
    char buf[N + 1]{};
    constexpr FixedString(char const* s) {
        for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
    }
    constexpr operator char const*() const { return buf; }
};
template <unsigned N>
FixedString(char const (&)[N]) -> FixedString<N - 1>;

enum class DebugLevel { DEBUG, INFO, WARN, ERROR };

namespace detail {
template <DebugLevel min_level, DebugLevel level>
concept ShouldEmitDebug = min_level <= level;

constexpr bek::str_view ansi_code_start(DebugLevel level) {
#ifndef NO_COLOUR
    switch (level) {
        case DebugLevel::DEBUG:
            return "\u001b[37m"_sv;
        case DebugLevel::INFO:
            return ""_sv;
        case DebugLevel::WARN:
            return "\u001b[93m"_sv;
        case DebugLevel::ERROR:
            return "\u001b[91m"_sv;
        default:
            return ""_sv;
    }
#else
    return ""_sv;
#endif
}

constexpr bek::str_view ansi_code_reset(DebugLevel level) {
#ifndef NO_COLOUR
    return "\u001b[0m"_sv;
#else
    return ""_sv;
#endif
}

}  // namespace detail

template <FixedString prefix, DebugLevel min_level>
struct DebugScope {
    constexpr static DebugLevel level = min_level;
    template <typename... Types>
    static void dbg(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void dbgln(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::DEBUG>
    static void dbg(bek::str_view format_str, Types const&... parameters) {
        if (debug_stream) {
            debug_stream->write(detail::ansi_code_start(DebugLevel::DEBUG));
            bek::format_to(*debug_stream, "[{}] "_sv, static_cast<const char*>(prefix));
            bek::format_to(*debug_stream, format_str, parameters...);
            debug_stream->write(detail::ansi_code_reset(DebugLevel::DEBUG));
        }
    }

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::DEBUG>
    static void dbgln(bek::str_view format_str, Types const&... parameters) {
        dbg(format_str, parameters...);
        if (debug_stream) {
            debug_stream->write('\n');
        }
    }

    template <typename... Types>
    static void info(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void infoln(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::INFO>
    static void info(bek::str_view format_str, Types const&... parameters) {
        if (debug_stream) {
            debug_stream->write(detail::ansi_code_start(DebugLevel::INFO));
            bek::format_to(*debug_stream, "[{}] "_sv, static_cast<const char*>(prefix));
            bek::format_to(*debug_stream, format_str, parameters...);
            debug_stream->write(detail::ansi_code_reset(DebugLevel::INFO));
        }
    }

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::INFO>
    static void infoln(bek::str_view format_str, Types const&... parameters) {
        info(format_str, parameters...);
        if (debug_stream) {
            debug_stream->write('\n');
        }
    }

    template <typename... Types>
    static void warn(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void warnln(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::WARN>
    static void warn(bek::str_view format_str, Types const&... parameters) {
        if (debug_stream) {
            debug_stream->write(detail::ansi_code_start(DebugLevel::WARN));
            bek::format_to(*debug_stream, "[{}] "_sv, static_cast<const char*>(prefix));
            bek::format_to(*debug_stream, format_str, parameters...);
            debug_stream->write(detail::ansi_code_reset(DebugLevel::WARN));
        }
    }

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::WARN>
    static void warnln(bek::str_view format_str, Types const&... parameters) {
        warn(format_str, parameters...);
        if (debug_stream) {
            debug_stream->write('\n');
        }
    }

    template <typename... Types>
    static void err(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void errln(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::ERROR>
    static void err(bek::str_view format_str, Types const&... parameters) {
        if (debug_stream) {
            debug_stream->write(detail::ansi_code_start(DebugLevel::ERROR));
            bek::format_to(*debug_stream, "[{}] "_sv, static_cast<const char*>(prefix));
            bek::format_to(*debug_stream, format_str, parameters...);
            debug_stream->write(detail::ansi_code_reset(DebugLevel::ERROR));
        }
    }

    template <typename... Types>
        requires detail::ShouldEmitDebug<min_level, DebugLevel::ERROR>
    static void errln(bek::str_view format_str, Types const&... parameters) {
        err(format_str, parameters...);
        if (debug_stream) {
            debug_stream->write('\n');
        }
    }
};

#else

template <FixedString prefix, bool enabled>
struct DebugScope {
    template <typename... Types>
    static void dbg(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void dbgln(bek::str_view format_str, Types const&... parameters) {}
};

#endif

#endif  // BEKOS_DEBUG_H
