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

#include "mm/page_allocator.h"

#include "library/assertions.h"

/// 00 = free; 11 = reserved, not end; 10 = reserved, end.
constexpr u8 RESERVED_END     = 0b10;
constexpr u8 RESERVED_NOT_END = 0b11;

mem::RegionPageAllocator::RegionPageAllocator(PhysicalRegion region) : m_region{region} {
    ASSERT(region.page_aligned());
    // First, we need to work out how large our data area needs to be.
    auto page_number = region.size / PAGE_SIZE;
    // We need to round up here. One bit needed per page for each array.
    auto bytes_per_bitmap = (page_number - 1) / 8 + 1;
    auto pages_needed     = (bytes_per_bitmap * 2 - 1) / PAGE_SIZE + 1;

    // TODO: Map into virtual memory. (How do we do this without mem allocation!?)
    m_free_bitmap = bek::mut_buffer{reinterpret_cast<char*>(region.start.ptr), bytes_per_bitmap};
    m_continuation_bitmap = bek::mut_buffer{m_free_bitmap.end(), bytes_per_bitmap};

    mark_as_reserved(0, pages_needed);
}

void mem::RegionPageAllocator::mark_as_reserved(uSize index, uSize n_pages) {
    auto end = index + n_pages;

    for (; index < end; index++) {
        auto byte_index = index / 8;
        auto bit_index  = index % 8;
        m_free_bitmap.data()[byte_index] |= 1 << bit_index;
        if (index + 1 == end) {
            m_continuation_bitmap.data()[byte_index] &= ~(1 << bit_index);
        } else {
            m_continuation_bitmap.data()[byte_index] |= 1 << bit_index;
        }
    }
}

/// Warning - must be given valid index to first allocated page of get_region.
void mem::RegionPageAllocator::mark_region_as_free(uSize start_index) {
    m_last_freed = start_index;
    for (;; start_index++) {
        auto byte_index = start_index / 8;
        auto bit_index  = start_index % 8;
        // Clear reserved bit
        m_free_bitmap.data()[byte_index] &= ~(1 << bit_index);
        if (!(m_continuation_bitmap.data()[byte_index] & (1 << bit_index))) {
            return;
        }
    }
}
uSize mem::RegionPageAllocator::search_for_free(uSize n_pages) {
    auto page_number  = m_region.size / PAGE_SIZE;
    auto region_begin = m_last_freed;
    uSize region_size = 0;
    for (uSize index = region_begin; index < page_number; index++) {
        if (m_free_bitmap.data()[index / 8] & (1 << (index % 8))) {
            // Not free
            region_size  = 0;
            region_begin = index + 1;
        } else {
            region_size++;
            if (region_size == n_pages) {
                return region_begin;
            }
        }
    }
    return -1;
}
bek::optional<mem::PhysicalPtr> mem::RegionPageAllocator::allocate_pages(uSize n_pages) {
    auto region = search_for_free(n_pages);
    if (region != static_cast<uSize>(-1)) {
        mark_as_reserved(region, n_pages);
        return m_region.start.offset(region * PAGE_SIZE);
    } else {
        return {};
    }
}
void mem::RegionPageAllocator::free_region(mem::PhysicalPtr region) {
    auto index = (region.page_base().get() - m_region.start.get()) / PAGE_SIZE;
    mark_region_as_free(index);
}
void mem::RegionPageAllocator::mark_as_reserved(mem::PhysicalRegion region) {
    auto index = (region.start.page_base().get() - m_region.start.get()) / PAGE_SIZE;
    auto len   = region.size / PAGE_SIZE;
    VERIFY(index + len < m_region.size / PAGE_SIZE);
    mark_as_reserved(index, region.size / PAGE_SIZE);
}

void mem::PageAllocator::register_new_region(mem::PhysicalRegion region) {
    for (auto& o_region : m_phys_regions) {
        if (!o_region) {
            o_region = RegionPageAllocator{region};
            return;
        } else {
            VERIFY(!o_region->region().overlaps(region));
        }
    }
    PANIC("Registered too many physical regions.");
}
void mem::PageAllocator::mark_as_reserved(mem::PhysicalRegion region) {
    VERIFY(region.page_aligned());
    for (auto& o_region : m_phys_regions) {
        if (o_region && o_region->region().overlaps(region)) {
            // Mark as reserved.
            o_region->mark_as_reserved(region);
        }
    }
}

mem::PageAllocator kernel_page_allocator{};

mem::PageAllocator& mem::PageAllocator::the() { return kernel_page_allocator; }
bek::optional<mem::PhysicalRegion> mem::PageAllocator::allocate_region(uSize page_number) {
    for (auto& o_region : m_phys_regions) {
        if (!o_region) {
            return {};
        }
        if (auto allocation = o_region->allocate_pages(page_number)) {
            return {*allocation, page_number * PAGE_SIZE};
        }
    }
    return {};
}
void mem::PageAllocator::free_region(mem::PhysicalPtr start) {
    for (auto& o_region : m_phys_regions) {
        if (!o_region) {
            return;
        }
        if (o_region->region().contains(start)) {
            o_region->free_region(start);
            return;
        }
    }
    PANIC("Tried to free page region not in memory.");
}
