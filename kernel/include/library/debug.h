/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#include "library/format.h"

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

template <FixedString prefix, bool>
struct DebugScope;

template <FixedString prefix>
struct DebugScope<prefix, true> {
    template <typename... Types>
    static void dbg(bek::str_view format_str, Types const&... parameters) {
        if (debug_stream) {
            bek::format_to(*debug_stream, "[{}] "_sv, static_cast<const char*>(prefix));
            bek::format_to(*debug_stream, format_str, parameters...);
        }
    }

    template <typename... Types>
    static void dbgln(bek::str_view format_str, Types const&... parameters) {
        dbg(format_str, parameters...);
        if (debug_stream) {
            debug_stream->write('\n');
        }
    }
};

template <FixedString prefix>
struct DebugScope<prefix, false> {
    template <typename... Types>
    static void dbg(bek::str_view format_str, Types const&... parameters) {}

    template <typename... Types>
    static void dbgln(bek::str_view format_str, Types const&... parameters) {}
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
