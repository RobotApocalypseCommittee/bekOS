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

#ifndef BEKOS_CORE_IO_H
#define BEKOS_CORE_IO_H

#include "bek/format.h"
#include "file.h"

namespace core {

extern core::BufferedFile stdout;
extern core::BufferedFile stderr;
extern core::BufferedFile stdin;

template <typename... Types>
void fprintln(core::BufferedFile& f, bek::str_view format_str, Types const&... parameters) {
    BufferedFileOutputStream s{f};
    bek::format_to(s, format_str, parameters...);
    s.write('\n');
    f.flush();
}

template <typename... Types>
void fprint(core::BufferedFile& f, bek::str_view format_str, Types const&... parameters) {
    BufferedFileOutputStream s{f};
    bek::format_to(s, format_str, parameters...);
    f.flush();
}

}  // namespace core

#endif  // BEKOS_CORE_IO_H
