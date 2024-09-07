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

#ifndef BEKOS_BYTE_FORMAT_H
#define BEKOS_BYTE_FORMAT_H

#include "bek/format.h"

namespace bek {

struct ByteSize {
    u64 underlying;
};

inline void bek_basic_format(OutputStream& out, const ByteSize& sz) {
    constexpr const uSize threshold = 8;
    auto n = sz.underlying;
    if (n >> 30 > threshold) {
        bek::format_to(out, "{}GiB"_sv, n >> 30);
    } else if (n >> 20 > threshold) {
        bek::format_to(out, "{}MiB"_sv, n >> 20);
    } else if (n >> 10 > threshold) {
        bek::format_to(out, "{}KiB"_sv, n >> 10);
    } else {
        bek::format_to(out, "{} bytes"_sv, n);
    }
}

}  // namespace bek

#endif  // BEKOS_BYTE_FORMAT_H
