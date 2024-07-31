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

#include "bek/assertions.h"

#include "library/format.h"

extern bek::OutputStream *debug_stream;

void bek::assertion_failed(const char *pExpr, const char *pFile, unsigned int nLine) {
    if (debug_stream) {
        bek::format_to(*debug_stream, "Assertion Failed: {} in {}, line {}."_sv, pExpr, pFile,
                       nLine);
    }
    panic();
}
void bek::panic(const char *message, const char *file_name, unsigned int line) {
    if (debug_stream) {
        bek::format_to(*debug_stream, "Panicked: {} in {}, line {}."_sv, message, file_name, line);
    }
    panic();
}
