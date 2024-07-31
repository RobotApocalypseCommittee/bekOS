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

#ifndef BEKOS_ARRAY_H
#define BEKOS_ARRAY_H

#include <initializer_list>

#include "bek/assertions.h"
#include "bek/span.h"
#include "bek/types.h"

namespace bek {

template <class T, uSize Size>
struct array {
    T _data[Size];

    [[nodiscard]] constexpr u64 size() const { return Size; }

    constexpr const T& operator[](u64 index) const {
        ASSERT(index < size());
        return _data[index];
    }

    constexpr T& operator[](u64 index) {
        ASSERT(index < size());
        return _data[index];
    }

    constexpr const T* data() const { return _data; }
    constexpr T* data() { return _data; }

    constexpr const T* begin() const { return data(); }

    constexpr const T* end() const { return data() + size(); }

    constexpr bek::span<T> span() { return {data(), size()}; }
};

// Template Deduction Guide: Array Literal
template <typename T, typename... U>
array(T, U...) -> array<T, 1 + sizeof...(U)>;
}  // namespace bek
#endif  // BEKOS_ARRAY_H
