/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2021  Bekos Inc Ltd
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

#ifndef BEKOS_HW_PTR_H
#define BEKOS_HW_PTR_H

#include <library/types.h>
#include <peripherals/peripherals.h>


#define INVALID_HW_PTR (-1)

template <typename T>
class hw_ptr {
public:
    hw_ptr() : raw_ptr(INVALID_HW_PTR) {}
    explicit hw_ptr(uPtr ptr) : raw_ptr(ptr) { assert((ptr & 0xFFFF000000000000) == 0); }

    uPtr get_raw_ptr() const {
        return raw_ptr;
    }

    uPtr get_mapped_ptr() const {
        return mapped_address(raw_ptr);
    }

    T& operator*() {
        return *(reinterpret_cast<T*>(mapped_address(raw_ptr)));
    }

    T* operator->() {
        return reinterpret_cast<T*>(mapped_address(raw_ptr));
    }

private:
    uPtr raw_ptr;
};

#endif  // BEKOS_HW_PTR_H
