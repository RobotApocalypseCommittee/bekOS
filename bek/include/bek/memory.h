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

#ifndef BEK_MEMORY_H
#define BEK_MEMORY_H

#include "types.h"

namespace bek {

/// Copies memory of length n bytes. Automatically handles memmove.
void memcopy(void* to, const void* from, uSize n);
void memset(void* dest, u8 value, uSize n);

uSize strlen(const char* str);
uSize strlen(const char* str, uSize max_len);

int mem_compare(const void* a, const void* b, uSize n);
}

#endif  // BEK_MEMORY_H
