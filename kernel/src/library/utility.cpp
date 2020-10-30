/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdint>
#include "library/utility.h"

template<>
u64 bek::hash<u64>(u64 x) {
    x ^= x >> 30u;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27u;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31u;
    return x;
}

void *operator new(uSize size) { return PREFIX(malloc)(size); }

void operator delete(void *ptr) { return PREFIX(free)(ptr); }

void operator delete(void *ptr, uSize) { return PREFIX(free)(ptr); }

void *operator new[](uSize size) { return PREFIX(malloc)(size); }

void operator delete[](void *ptr) { return PREFIX(free)(ptr); }

void operator delete[](void *ptr, size_t) { return PREFIX(free)(ptr); }

void *operator new(size_t, void *ptr) { return ptr; }

void *operator new[](size_t, void *ptr) { return ptr; }
