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

#ifndef BEK_NUMERIC_LIMITS_H
#define BEK_NUMERIC_LIMITS_H

namespace bek {

template <typename T>
struct numeric_limits {};

template <>
struct numeric_limits<unsigned char> {
    static constexpr uSize min() { return 0; }
    static constexpr uSize max() { return ~(unsigned char)0; }
};

template <>
struct numeric_limits<unsigned short> {
    static constexpr uSize min() { return 0; }
    static constexpr uSize max() { return ~(unsigned short)0; }
};

template <>
struct numeric_limits<unsigned int> {
    static constexpr uSize min() { return 0; }
    static constexpr uSize max() { return ~(unsigned int)0; }
};

template <>
struct numeric_limits<long> {
    static constexpr uSize max() { return (2ul << (sizeof(long) * 8ul - 1ul)) - 1ul; }
};

}  // namespace bek

#endif  // BEK_NUMERIC_LIMITS_H
