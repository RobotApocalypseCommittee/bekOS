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

#ifndef BEKOS_UTILITY_H
#define BEKOS_UTILITY_H
#include <type_traits>

#include "library/liballoc.h"
#include "types.h"

void *operator new(size_t, void *ptr);
void *operator new[](size_t, void *ptr);

namespace bek {

enum class endianness {
    native = __BYTE_ORDER__,
    big    = __ORDER_BIG_ENDIAN__,
    little = __ORDER_LITTLE_ENDIAN__
};

constexpr bool IS_LITTLE_ENDIAN = endianness::native == endianness::little;

template <typename T>
inline T swapBytes(T value);

template <>
inline u32 swapBytes<u32>(u32 value) {
    return __builtin_bswap32(value);
}

template <>
inline u16 swapBytes<u16>(u16 value) {
    return __builtin_bswap16(value);
}

template <typename T>
inline T readLE(const u8 *data);

template <>
inline u32 readLE<u32>(const u8 *data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        return *reinterpret_cast<const u32 *>(data);
    } else {
        return data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
    }
}

template <>
inline u16 readLE<u16>(const u8 *data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        return *reinterpret_cast<const u16 *>(data);
    } else {
        return data[0] | data[1] << 8;
    }
}

template <typename T>
inline void writeLE(T value, u8 *data) = delete;

template <>
inline void writeLE<u32>(u32 value, u8 *data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        *reinterpret_cast<u32 *>(data) = value;
    } else {
        *reinterpret_cast<u32 *>(data) = swapBytes<u32>(value);
    }
}

template <>
inline void writeLE<u16>(u16 value, u8 *data) {
    if constexpr (IS_LITTLE_ENDIAN) {
        *reinterpret_cast<u16 *>(data) = value;
    } else {
        *reinterpret_cast<u16 *>(data) = swapBytes<u16>(value);
    }
}

template <class T>
struct remove_reference {
    typedef T type;
};
template <class T>
struct remove_reference<T &> {
    typedef T type;
};
template <class T>
struct remove_reference<T &&> {
    typedef T type;
};

template <class T>
typename remove_reference<T>::type &&move(T &&t) noexcept {
    return static_cast<typename remove_reference<T>::type &&>(t);
}

template <class T>
constexpr T &&forward(typename remove_reference<T>::type &t) noexcept {
    return static_cast<T &&>(t);
}

template <class T>
constexpr T &&forward(typename remove_reference<T>::type &&t) noexcept {
    static_assert(!std::is_lvalue_reference<T>::value, "can not forward an rvalue as an lvalue");
    return static_cast<T &&>(t);
}

template <class T>
void swap(T &a, T &b) noexcept {
    T tmp = move(a);
    a     = move(b);
    b     = move(tmp);
}

template <class T>
u64 hash(T);

template <>
u64 hash(u64 x);

template <class T>
void copy(T *dest, const T *src, size_t n) {
    while (n > 0) {
        n--;
        *(dest++) = *(src++);
    }
}

template <class T>
void copy_backward(T *dest, const T *src, size_t n) {
    dest += n - 1;
    src += n - 1;
    while (n > 0) {
        n--;
        *(dest--) = *(src--);
    }
}

template <typename T>
inline T min(T a, T b) {
    return a > b ? b : a;
}

template <typename T>
inline T max(T a, T b) {
    return a > b ? a : b;
}

template <class T, class U>
struct pair {
    T first;
    U second;
};
}

#endif //BEKOS_UTILITY_H
