/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_UTILITY_H
#define BEKOS_UTILITY_H

#include "kstring.h"
#include "traits.h"
#include "types.h"

namespace bek {

enum class endianness {
    native = __BYTE_ORDER__,
    big    = __ORDER_BIG_ENDIAN__,
    little = __ORDER_LITTLE_ENDIAN__
};

constexpr inline bool IS_LITTLE_ENDIAN = endianness::native == endianness::little;

template <typename T>
inline T swapBytes(T value);

template <>
inline u64 swapBytes<u64>(u64 value) {
    return __builtin_bswap64(value);
}

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


template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<class T, T v>
struct integral_constant {
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant; // using injected-class-name
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; } //since c++14
};

using false_type = integral_constant<bool, false>;
using true_type = integral_constant<bool, true>;

namespace detail {

template <bool B, typename T, typename F>
struct conditional {
    using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

}  // namespace detail

template <bool B, typename T, typename F>
using conditional = detail::conditional<B, T, F>::type;

template<class T> struct is_lvalue_reference     : false_type {};
template<class T> struct is_lvalue_reference<T&> : true_type {};

template <class T>
remove_reference<T> &&move(T &&t) noexcept {
    return static_cast<remove_reference<T> &&>(t);
}

template <class T>
constexpr T &&forward(remove_reference<T> &t) noexcept {
    return static_cast<T &&>(t);
}

template <class T>
constexpr T &&forward(remove_reference<T> &&t) noexcept {
    static_assert(!is_lvalue_reference<T>::value, "can not forward an rvalue as an lvalue");
    return static_cast<T &&>(t);
}

template <class T, typename U = T>
constexpr T exchange(T &obj, U &&u) {
    T old = bek::move(obj);
    obj   = bek::forward<U>(u);
    return old;
}

template <class T>
void swap(T &a, T &b) noexcept {
    T tmp = move(a);
    a     = move(b);
    b     = move(tmp);
}

u64 hash(u64 x);

inline u64 hash(u32 x) { return hash(static_cast<u64>(x)); }

template <class T>
void copy(T *dest, const T *src, uSize n) {
    while (n > 0) {
        n--;
        *(dest++) = *(src++);
    }
}

template <class T>
void copy_backward(T *dest, const T *src, uSize n) {
    dest += n - 1;
    src += n - 1;
    while (n > 0) {
        n--;
        *(dest--) = *(src--);
    }
}

template <typename T>
constexpr T min(T a, T b) {
    return a > b ? b : a;
}

template <typename T>
constexpr T max(T a, T b) {
    return a > b ? a : b;
}

template <typename T>
constexpr T ceil_div(T a, T b) {
    return (a + b - 1) / b;
}

template <typename T>
constexpr T align_up(T a, T alignment) {
    return ceil_div(a, alignment) * alignment;
}

template <typename T>
constexpr T align_down(T a, T alignment) {
    return (a / alignment) * alignment;
}

template <class T, class U>
struct pair {
    T first;
    U second;
};

template <uSize Size, uSize Align>
struct aligned_storage {
    alignas(Align) char data[Size];

    [[nodiscard]] constexpr const void *ptr() const { return &data[0]; }

    [[nodiscard]] constexpr void *ptr() { return &data[0]; }

    template <typename T>
    static constexpr bool can_store =
        sizeof(T) <= Size && alignof(T) <= Align && Align % alignof(T) == 0;
};

template <bek::trivially_copyable To, bek::trivially_copyable From>
    requires(sizeof(To) == sizeof(From))
constexpr To bit_cast(const From &from) noexcept {
    return __builtin_bit_cast(To, from);
}

constexpr u32 floor_log_2(u32 n) {
    if (n == 0) return n;
    return 31u - __builtin_clz(n);
}

}

#endif //BEKOS_UTILITY_H
