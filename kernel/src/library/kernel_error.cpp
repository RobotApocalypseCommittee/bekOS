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

#include "library/kernel_error.h"

#include "bek/format.h"

void bek_basic_format(bek::OutputStream& out, const ErrorCode& code) {
    switch (code) {
#define __EMIT_ERROR_CODE(CODE) \
    case CODE:                  \
        out.write(#CODE##_sv);  \
        break;
        __ENUMERATE_ERROR_CODES
#undef __EMIT_ERROR_CODE
    }
}
