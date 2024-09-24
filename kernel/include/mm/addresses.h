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

#include "arch/a64/memory_constants.h"
#include "bek/compare.h"
#include "bek/format_core.h"
#include "bek/optional.h"
#include "bek/types.h"
#include "bek/utility.h"

namespace mem {
constexpr inline uPtr PAGE_MASK        = ~(PAGE_SIZE - 1);
constexpr inline uPtr PAGE_OFFSET_MASK = PAGE_SIZE - 1;

void format_pointer(bek::OutputStream&, uPtr);

template <typename StrongType>
struct strong_ptr {
    uPtr ptr;

    constexpr strong_ptr() : ptr() {};
    constexpr explicit strong_ptr(uPtr a) : ptr(a) {};

    [[nodiscard]] constexpr uPtr get() const { return ptr; }
    [[nodiscard]] constexpr StrongType offset(iSize byte_offset) const {
        // TODO: Check non-overflowing.
        return StrongType{ptr + byte_offset};
    }

    [[nodiscard]] constexpr StrongType page_base() const { return {ptr - (ptr % PAGE_SIZE)}; }

    [[nodiscard]] constexpr uSize page_offset() const { return ptr % PAGE_SIZE; }

    explicit constexpr operator uPtr&() noexcept { return ptr; }
    explicit constexpr operator const uPtr&() const noexcept { return ptr; }

    friend constexpr bool operator==(StrongType a, StrongType b) { return a.ptr == b.ptr; };

    friend constexpr bool operator!=(StrongType a, StrongType b) { return a.ptr != b.ptr; }

    friend auto operator<=>(StrongType a, StrongType b) { return a.ptr <=> b.ptr; }

    friend constexpr StrongType operator+(StrongType a, iSize b) { return a.offset(b); }

    friend constexpr StrongType& operator+=(StrongType& a, iSize b) {
        a = a.offset(b);
        return a;
    }

    friend constexpr iSize operator-(StrongType a, StrongType b) { return a.ptr - b.ptr; }

    friend constexpr StrongType operator-(StrongType a, iSize b) { return a.offset(-b); }

    friend constexpr StrongType& operator-=(StrongType& a, iSize b) {
        a = a.offset(-b);
        return a;
    }

    friend void bek_basic_format(bek::OutputStream& o, const StrongType& p) { format_pointer(o, p.ptr); }
};

template <typename StrongRegion, typename StrongPtr>
struct strong_ptr_region {
    StrongPtr start;
    uSize size;

    strong_ptr_region(uPtr start, uSize size) : start{start}, size{size} {};
    strong_ptr_region(StrongPtr start, uSize size) : start{start}, size{size} {};

    [[nodiscard]] constexpr StrongPtr end() const { return start.offset(size); }

    [[nodiscard]] constexpr bool overlaps(StrongRegion other) const {
        return (other.start < end()) && (other.end() > start);
    }

    [[nodiscard]] constexpr bool contains(StrongPtr ptr) const { return (ptr >= start) && (ptr < start.offset(size)); }

    [[nodiscard]] constexpr bool contains(StrongRegion other) const {
        return (other.start >= start && (other.end() <= end()));
    }

    [[nodiscard]] constexpr StrongRegion intersection(StrongRegion other) const {
        StrongPtr new_start = bek::max(start, other.start);
        StrongPtr new_end = bek::min(end(), other.end());
        if (new_end >= new_start) {
            return {new_start, new_end.ptr - new_start.ptr};
        } else {
            return {{0}, 0};
        }
    }

    [[nodiscard]] constexpr bool page_aligned() const { return start.page_offset() == 0 && (size % PAGE_SIZE == 0); }

    [[nodiscard]] constexpr StrongRegion align_to_page() const {
        auto end_address = bek::align_up(end().get(), (uPtr)PAGE_SIZE);
        auto start_address = bek::align_down(start.get(), (uPtr)PAGE_SIZE);
        return {{start_address}, end_address - start_address};
    }
};

struct PhysicalPtr : strong_ptr<PhysicalPtr> {
    using strong_ptr::strong_ptr;
};

struct PhysicalRegion : strong_ptr_region<PhysicalRegion, PhysicalPtr> {
    using strong_ptr_region::strong_ptr_region;
};

struct DmaPtr : strong_ptr<DmaPtr> {
    using strong_ptr::strong_ptr;
};

struct UserPtr : strong_ptr<UserPtr> {
    using strong_ptr::strong_ptr;
};

struct UserRegion : strong_ptr_region<UserRegion, UserPtr> {
    using strong_ptr_region::strong_ptr_region;
};

using KernelPtr = u8*;

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

using PhysPtr = mem::PhysicalPtr;

#endif  // BEKOS_ADDRESSES_H
