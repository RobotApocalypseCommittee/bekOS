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

#ifndef BEK_BITWISE_ENUM_H
#define BEK_BITWISE_ENUM_H

template <typename T>
constexpr inline bool bek_is_bitwise_enum = false;

template <typename T>
concept bitwise_enum = __is_enum(T) && bek_is_bitwise_enum<T>;

#define BEK_UNDERLYING_TYPE(BEK_ENUM_T) __underlying_type(BEK_ENUM_T)

template <bitwise_enum enum_t>
[[gnu::always_inline]] constexpr auto operator|(const enum_t& lhs, const enum_t& rhs) {
    return static_cast<enum_t>(static_cast<BEK_UNDERLYING_TYPE(enum_t)>(lhs) |
                               static_cast<BEK_UNDERLYING_TYPE(enum_t)>(rhs));
}

template <bitwise_enum enum_t>
[[gnu::always_inline]] constexpr auto operator&(const enum_t& lhs, const enum_t& rhs) {
    return static_cast<enum_t>(static_cast<BEK_UNDERLYING_TYPE(enum_t)>(lhs) &
                               static_cast<BEK_UNDERLYING_TYPE(enum_t)>(rhs));
}

template <bitwise_enum enum_t>
[[gnu::always_inline]] constexpr void operator|=(const enum_t& lhs, const enum_t& rhs) {
    lhs = lhs | rhs;
}

template <bitwise_enum enum_t>
[[gnu::always_inline]] constexpr void operator&=(const enum_t& lhs, const enum_t& rhs) {
    lhs = lhs & rhs;
}

template <bitwise_enum enum_t>
[[gnu::always_inline]] constexpr bool operator!(const enum_t& val) {
    return static_cast<BEK_UNDERLYING_TYPE(enum_t)>(val) == static_cast<BEK_UNDERLYING_TYPE(enum_t)>(0);
}

#endif  // BEK_BITWISE_ENUM_H
