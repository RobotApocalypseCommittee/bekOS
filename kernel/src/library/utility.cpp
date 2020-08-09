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

#include "library/utility.h"

enum class endianness {
    native = __BYTE_ORDER__,
    big = __ORDER_BIG_ENDIAN__,
    little = __ORDER_LITTLE_ENDIAN__
};

constexpr bool IS_LITTLE_ENDIAN = endianness::native == endianness::little;

template<>
u64 bek::hash<u64>(u64 x) {
    x ^= x >> 30u;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27u;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31u;
    return x;
}

template<>
u32 bek::readLE<u32>(const u8* data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        return *reinterpret_cast<const u32*>(data);
    } else {
        return data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
    }
}

template<>
u16 bek::readLE<u16>(const u8* data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        return *reinterpret_cast<const u16*>(data);
    } else {
        return data[0] | data[1] << 8;
    }
}

template<>
void bek::writeLE<u32>(u32 value, u8* data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        *reinterpret_cast<u32*>(data) = value;
    } else {
        *reinterpret_cast<u32*>(data) = swapBytes<u32>(value);
    }
}

template<>
void bek::writeLE<u16>(u16 value, u8* data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        *reinterpret_cast<u16*>(data) = value;
    } else {
        *reinterpret_cast<u16*>(data) = swapBytes<u16>(value);
    }
}

template<>
u32 bek::swapBytes<u32>(u32 value) {
    return __builtin_bswap32(value);
}

template<>
u16 bek::swapBytes<u16>(u16 value) {
    return __builtin_bswap16(value);
}
