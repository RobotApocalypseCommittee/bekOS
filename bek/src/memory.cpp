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

#include "bek/memory.h"

#include "bek/assertions.h"

inline constexpr uSize MAX_MEMCOPY = 2 * MiB;

extern "C" {

using size_t = decltype(sizeof(0));
// Defined in assembly.
void* memcpy(void* __restrict s1, const void* __restrict s2, size_t n);
void* memmove(void* s1, const void* s2, size_t n);

void* memset(void* s, int c, size_t n) {
    auto* d = reinterpret_cast<unsigned char*>(s);
    u8 value = c;
    // If 64-bit aligned.
    if ((reinterpret_cast<uPtr>(d) & 7) == 0) {
        // Repeats value 8 times.
        u64 bit_mask = ((u64)-1) / 255 * value;
        for (; n >= 8; n -= 8) {
            *reinterpret_cast<u64*>(d) = bit_mask;
            d += 8;
        }
    }
    for (; n > 0; n--) {
        *(d++) = value;
    }
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    auto* q1 = static_cast<const u8*>(s1);
    auto* q2 = static_cast<const u8*>(s2);
    int d = 0;
    while (n--) {
        d = *q1++ - *q2++;
        if (d) {
            return d;
        }
    }
    return d;
}
}  // extern "C"

void bek::memcopy(void* to, const void* from, uSize n) {
    if (n == 0) return;
    VERIFY(n <= MAX_MEMCOPY);
    if (((char*)to - (const char*)from) <= (iSize)n) {
        memcpy(to, from, n);
    } else {
        memmove(to, from, n);
    }
}

void bek::memset(void* dest, u8 value, uSize n) { ::memset(dest, value, n); }

int bek::mem_compare(const void* a, const void* b, uSize n) { return ::memcmp(a, b, n); }

uSize bek::strlen(const char* str) {
    const char* ss = str;
    while (*ss) {
        ss++;
    }
    return ss - str;
}

uSize bek::strlen(const char* str, uSize max_len) {
    const char* max_ptr = str + max_len;
    const char* cur_ptr = str;
    while (*cur_ptr && cur_ptr < max_ptr) {
        cur_ptr++;
    }
    return cur_ptr - str;
}
