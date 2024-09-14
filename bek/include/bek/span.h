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
#ifndef BEK_SPAN_H
#define BEK_SPAN_H

#include "assertions.h"
#include "types.h"
#include "vector.h"

namespace bek {

// An array view with length
template <typename T>
class span {
public:
    span(T* array, uSize length) : array(array), length(length) {}

    span(const T* begin, const T* end) : array(begin), length(end - begin) {}

    span() : array(nullptr), length(0) {}

    explicit span(const vector<T>& v) : array(v.data()), length(v.size()) {}
    template <uSize N>
    span(const T (&arr)[N]) : array(&arr[0]), length(N) {}

    const T& operator[](uSize idx) const {
        ASSERT(idx < length);
        return array[idx];
    }

    const T* data() const { return array; }
    const T* begin() { return data(); }
    const T* end() { return &data()[size()]; }

    uSize size() const { return length; }

private:
    const T* array;
    uSize length;
};

}  // namespace bek
#endif  // BEK_SPAN_H
