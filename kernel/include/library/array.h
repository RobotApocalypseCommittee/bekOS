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

#ifndef BEKOS_ARRAY_H
#define BEKOS_ARRAY_H
#include "hardtypes.h"
#include "library/assert.h"


namespace bek {

    template<class T, u64 _size>
    class arr {
    public:
        inline constexpr u64 size() {
            return _size;
        }

        inline T& operator[](u64 index) {
            assert(index < size());
            return _data[index];
        }

        inline T* data() {
            return _data;
        }

    private:
        T _data[_size];
    };
}
#endif //BEKOS_ARRAY_H
