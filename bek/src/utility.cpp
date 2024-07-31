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

#include "bek/utility.h"

u64 bek::hash(u64 x) {
    x ^= x >> 30u;
    x *= 0xbf58476d1ce4e5b9ul;
    x ^= x >> 27u;
    x *= 0x94d049bb133111ebul;
    x ^= x >> 31u;
    return x;
}
u64 bek::hash(const char* str, uSize len) {
    u64 h = 37;
    for (uSize i = 0; i < len; i++) {
        h = (h * 54059) ^ (str[i] * 76963);
    }
    return h;
}
void bek::memcopy(void* to, const void* from, uSize n) {
    auto* src  = reinterpret_cast<const u8*>(from);
    auto* dest = reinterpret_cast<u8*>(to);
    for (uSize i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}
void bek::memmove(void* to, const void* from, uSize n) {
    auto* src  = reinterpret_cast<const u8*>(from);
    auto* dest = reinterpret_cast<u8*>(to);
    // Bad overlap.
    if (src < dest && (src + n) > dest) {
        while (n) {
            dest[n - 1] = src[n - 1];
            n--;
        }
    }
}
uSize bek::strlen(const char* str) {
    const char* ss = str;
    while (*ss) {
        ss++;
    }
    return ss - str;
}
void bek::memset(void* dest, u8 value, uSize n) {
    u8* cur = static_cast<u8*>(dest);
    for (uSize i = 0; i < n; i++) {
        cur[i] = value;
    }
}
uSize bek::strnlen(const char* str, uSize max_len) {
    const char* ss = str;
    while (*ss && max_len) {
        ss++;
        max_len--;
    }
    return ss - str;
}
