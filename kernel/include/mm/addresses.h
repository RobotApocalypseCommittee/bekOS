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

#ifndef BEKOS_ADDRESSES_H
#define BEKOS_ADDRESSES_H

#include <compare>

#include "arch/a64/memory_constants.h"
#include "bek/types.h"
#include "bek/utility.h"
#include "library/optional.h"

namespace mem {
constexpr inline uPtr PAGE_MASK        = ~(PAGE_SIZE - 1);
constexpr inline uPtr PAGE_OFFSET_MASK = PAGE_SIZE - 1;

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

struct VirtualPtr {
    u8* ptr;

    [[nodiscard]] constexpr void* get() const { return ptr; }
    [[nodiscard]] constexpr u8* get_bytes() const { return ptr; }
    [[nodiscard]] constexpr VirtualPtr offset(uSize byte_offset) const {
        return {ptr + byte_offset};
    }
    [[nodiscard]] uPtr raw() const { return reinterpret_cast<uPtr>(ptr); }

    [[nodiscard]] constexpr VirtualPtr page_base() const {
        return {bek::bit_cast<u8*>((bek::bit_cast<uPtr>(ptr) / PAGE_SIZE) * PAGE_SIZE)};
    }

    [[nodiscard]] constexpr uSize page_offset() const {
        return bek::bit_cast<uPtr>(ptr) % PAGE_SIZE;
    }

    friend bool operator==(VirtualPtr, VirtualPtr)  = default;
    friend bool operator!=(VirtualPtr, VirtualPtr)  = default;
    friend auto operator<=>(VirtualPtr, VirtualPtr) = default;
    friend constexpr iPtr operator-(VirtualPtr a, VirtualPtr b) { return (a.ptr - b.ptr); }
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

    [[nodiscard]] constexpr bool contains(PhysicalRegion other) const {
        return (other.start >= start && (other.end() <= end()));
    }

    [[nodiscard]] constexpr PhysicalRegion intersection(PhysicalRegion other) const {
        PhysicalPtr new_start = bek::max(start, other.start);
        PhysicalPtr new_end   = bek::min(end(), other.end());
        if (new_end >= new_start) {
            return {new_start, new_end.ptr - new_start.ptr};
        } else {
            return {{0}, 0};
        }
    }

    [[nodiscard]] constexpr bool page_aligned() const {
        return start.page_offset() == 0 && (size % PAGE_SIZE == 0);
    }

    [[nodiscard]] constexpr PhysicalRegion align_to_page() const {
        auto end_address   = bek::align_up(end().get(), (uPtr)PAGE_SIZE);
        auto start_address = bek::align_down(start.get(), (uPtr)PAGE_SIZE);
        return {{start_address}, end_address - start_address};
    }
};

struct VirtualRegion {
    VirtualPtr start;
    uSize size;

    [[nodiscard]] constexpr VirtualPtr end() const { return start.offset(size); }

    [[nodiscard]] constexpr bool overlaps(VirtualRegion other) const {
        return (other.start < end()) && (other.end() > start);
    }

    [[nodiscard]] constexpr bool contains(VirtualPtr ptr) const {
        return (ptr >= start) && (ptr < start.offset(size));
    }

    [[nodiscard]] constexpr bool contains(VirtualRegion other) const {
        return (other.start >= start && (other.end() <= end()));
    }

    [[nodiscard]] constexpr VirtualRegion intersection(VirtualRegion other) const {
        VirtualPtr new_start = bek::max(start, other.start);
        VirtualPtr new_end   = bek::min(end(), other.end());
        if (new_end >= new_start) {
            return {new_start, static_cast<uSize>(new_end.ptr - new_start.ptr)};
        } else {
            return {{nullptr}, 0};
        }
    }

    [[nodiscard]] constexpr bool page_aligned() const {
        return start.page_offset() == 0 && (size % PAGE_SIZE == 0);
    }
};

bek::optional<PhysicalPtr> kernel_virt_to_phys(void* ptr);
void* kernel_phys_to_virt(PhysicalPtr ptr);

}  // namespace mem

#endif  // BEKOS_ADDRESSES_H
