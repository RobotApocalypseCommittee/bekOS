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

#ifndef BEK_COMPARE_H
#define BEK_COMPARE_H

// NOLINTBEGIN(*-dcl58-cpp)
namespace std {

struct strong_ordering;

struct strong_ordering {
public:
    constexpr explicit strong_ordering(signed char c) : m_value{c} {}

    static const strong_ordering less;
    static const strong_ordering greater;
    static const strong_ordering equal;
    static const strong_ordering equivalent;

    friend constexpr bool operator==(strong_ordering, strong_ordering) noexcept = default;

    friend constexpr bool operator==(strong_ordering o, decltype(nullptr)) noexcept { return o.m_value == 0; }

    friend constexpr bool operator<(strong_ordering o, decltype(nullptr)) noexcept { return o.m_value < 0; }

    friend constexpr bool operator<=(strong_ordering o, decltype(nullptr)) noexcept { return o.m_value <= 0; }

    friend constexpr bool operator>=(strong_ordering o, decltype(nullptr)) noexcept { return o.m_value >= 0; }

    friend constexpr bool operator>(strong_ordering o, decltype(nullptr)) noexcept { return o.m_value > 0; }

    friend constexpr bool operator==(decltype(nullptr), strong_ordering o) noexcept { return 0 == o.m_value; }

    friend constexpr bool operator<(decltype(nullptr), strong_ordering o) noexcept { return 0 < o.m_value; }
    friend constexpr bool operator<=(decltype(nullptr), strong_ordering o) noexcept { return 0 <= o.m_value; }
    friend constexpr bool operator>(decltype(nullptr), strong_ordering o) noexcept { return 0 > o.m_value; }
    friend constexpr bool operator>=(decltype(nullptr), strong_ordering o) noexcept { return 0 >= o.m_value; }

private:
    signed char m_value;
};

inline constexpr strong_ordering strong_ordering::less{-1};
inline constexpr strong_ordering strong_ordering::greater{1};
inline constexpr strong_ordering strong_ordering::equal{0};
inline constexpr strong_ordering strong_ordering::equivalent{-1};

}  // namespace std
// NOLINTEND(*-dcl58-cpp)

#endif  // BEK_COMPARE_H