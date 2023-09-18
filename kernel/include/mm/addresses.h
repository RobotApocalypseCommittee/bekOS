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

#ifndef BEKOS_ADDRESSES_H
#define BEKOS_ADDRESSES_H

#include <compare>

#include "library/types.h"

namespace mem {
constexpr uSize PAGE_SIZE = 4096;

struct PhysicalPtr {
    uPtr ptr;

    [[nodiscard]] constexpr uPtr get() const { return ptr; }
    [[nodiscard]] constexpr PhysicalPtr offset(uSize byte_offset) const {
        return {ptr + byte_offset};
    }

    [[nodiscard]] constexpr PhysicalPtr page_base() const {
        return {(ptr / PAGE_SIZE) * PAGE_SIZE};
    }

    [[nodiscard]] constexpr uSize page_offset() const { return ptr % PAGE_SIZE; }

    friend bool operator==(PhysicalPtr, PhysicalPtr)  = default;
    friend bool operator!=(PhysicalPtr, PhysicalPtr)  = default;
    friend auto operator<=>(PhysicalPtr, PhysicalPtr) = default;
};

struct DmaPtr {
    uPtr ptr;

    [[nodiscard]] constexpr uPtr get() const { return ptr; }
    [[nodiscard]] constexpr DmaPtr offset(uSize byte_offset) const { return {ptr + byte_offset}; }

    [[nodiscard]] constexpr DmaPtr page_base() const { return {(ptr / PAGE_SIZE) * PAGE_SIZE}; }

    [[nodiscard]] constexpr uSize page_offset() const { return ptr % PAGE_SIZE; }

    friend bool operator==(DmaPtr, DmaPtr)  = default;
    friend bool operator!=(DmaPtr, DmaPtr)  = default;
    friend auto operator<=>(DmaPtr, DmaPtr) = default;
};

struct PhysicalRegion {
    PhysicalPtr start;
    uSize size;

    [[nodiscard]] constexpr PhysicalPtr end() const { return start.offset(size); }

    [[nodiscard]] constexpr bool overlaps(PhysicalRegion other) const {
        return (other.start < end()) && (other.end() > start);
    }

    [[nodiscard]] constexpr bool contains(PhysicalPtr ptr) const {
        return (ptr >= start) && (ptr < start.offset(size));
    }

    [[nodiscard]] constexpr bool page_aligned() const {
        return start.page_offset() == 0 && (size % PAGE_SIZE == 0);
    }
};

}  // namespace mem

#endif  // BEKOS_ADDRESSES_H
