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

#ifndef BEKOS_INITIALIZER_LIST_H
#define BEKOS_INITIALIZER_LIST_H

#include "bek/types.h"

// Janky

namespace std {

template <typename T>
class initializer_list {
public:
    using value_type      = T;
    using reference       = const T&;
    using const_reference = const T&;
    using size_type       = uSize;
    using iterator        = const T*;
    using const_iterator  = const T*;

    constexpr initializer_list() noexcept = default;

private:
    const T* m_begin{nullptr};
    uSize m_size{0};

public:
    constexpr initializer_list(const T* begin, uSize size) noexcept
        : m_begin(begin), m_size(size) {}
    constexpr size_type size() const noexcept { return m_size; }
    constexpr iterator begin() const noexcept { return m_begin; }
    constexpr iterator end() const noexcept { return m_begin + m_size; }
};

}  // namespace std

#endif  // BEKOS_INITIALIZER_LIST_H
