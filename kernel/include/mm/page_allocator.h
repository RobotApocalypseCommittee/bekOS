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

#ifndef BEKOS_PAGE_ALLOCATOR_H
#define BEKOS_PAGE_ALLOCATOR_H

#include "addresses.h"
#include "bek/buffer.h"
#include "bek/types.h"
#include "library/optional.h"
namespace mem {

/// Allocator for a contiguous region of pages. Stores allocation data within its own get_region.
class RegionPageAllocator {
public:
    explicit RegionPageAllocator(VirtualRegion region);

    /// @brief Allocates contiguous get_region of `n_pages` pages.
    /// @returns Physical Pointer to first page, or -1 if failed.
    bek::optional<VirtualPtr> allocate_pages(uSize n_pages);

    void mark_as_reserved(VirtualRegion region);

    /// @brief Frees a region allocated. UB if get_region is not valid.
    /// \param region Physical Pointer to first page of get_region.
    void free_region(VirtualPtr region);

    [[nodiscard]] VirtualRegion region() const { return m_region; }

private:
    void mark_as_reserved(uSize index, uSize n_pages);
    void mark_region_as_free(uSize start_index);
    uSize search_for_free(uSize n_pages);

    VirtualRegion m_region;

    bek::mut_buffer m_free_bitmap{nullptr, 0};
    bek::mut_buffer m_continuation_bitmap{nullptr, 0};

    uSize m_last_freed{0};
};

/// Allocator to request (strings of) physical pages of memory.
class PageAllocator {
public:
    /// Registers a new region of continuous physical memory.
    /// \param region Region to register; must be page-aligned and may not overlap with any existing
    /// regions.
    void register_new_region(VirtualRegion region);

    /// Marks a region (of physical pages) as reserved.
    /// \param region Region to mark as reserved; must be page-aligned, and completely contained
    /// within an existing region. \note Erroneous code could free reserved regions.
    void mark_as_reserved(VirtualRegion region);

    /// Tries to allocate a contiguous region of pages `page_number` long.
    /// \param page_number Number of pages to attempt to allocate.
    /// \return Region allocated, if successful. nullopt if not.
    bek::optional<VirtualRegion> allocate_region(uSize page_number);

    /// Frees a region of contiguous pages starting with start.
    /// \param start Start pointer. Error is start does not refer to the start of a region allocated
    /// by allocate_region.
    void free_region(VirtualPtr start);

    static PageAllocator& the();

private:
    static constexpr uSize MAX_PHYSICAL_REGIONS = 4;
    bek::optional<RegionPageAllocator> m_phys_regions[MAX_PHYSICAL_REGIONS];
};

}  // namespace mem

#endif  // BEKOS_PAGE_ALLOCATOR_H
