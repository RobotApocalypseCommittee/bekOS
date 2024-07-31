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

#ifndef BEKOS_NUMERIC_LIMITS_H
#define BEKOS_NUMERIC_LIMITS_H

namespace bek {

template <typename T>
struct numeric_limits {};

template <>
struct numeric_limits<unsigned char> {
    static constexpr unsigned char min() { return 0; }
    static constexpr unsigned char max() { return ~(unsigned char)0; }
};

template <>
struct numeric_limits<unsigned short> {
    static constexpr unsigned short min() { return 0; }
    static constexpr unsigned short max() { return ~(unsigned short)0; }
};

template <>
struct numeric_limits<unsigned int> {
    static constexpr unsigned int min() { return 0; }
    static constexpr unsigned int max() { return ~(unsigned int)0; }
};

}  // namespace bek

#endif  // BEKOS_NUMERIC_LIMITS_H
