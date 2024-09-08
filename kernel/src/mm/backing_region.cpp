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

#include "mm/backing_region.h"

#include "mm/page_allocator.h"
#include "mm/space_manager.h"

expected<bek::shared_ptr<mem::UserOwnedAllocation>> mem::UserOwnedAllocation::create_contiguous(uSize pages) {
    auto allocation = mem::PageAllocator::the().allocate_region(pages);
    if (!allocation) return ENOMEM;
    auto phys_ptr = mem::kernel_virt_to_phys(allocation->start.get());
    VERIFY(phys_ptr);
    return bek::adopt_shared(new UserOwnedAllocation(*allocation, *phys_ptr));
}

ErrorCode mem::UserOwnedAllocation::map_into_table(TableManager& manager, mem::UserRegion user_region, uSize offset,
                                                   bool readable, bool writeable, bool executable) {
    VERIFY(user_region.page_aligned() && (offset % PAGE_SIZE) == 0 && (user_region.size + offset) <= m_region.size);

    if (!manager.map_region(user_region.start.ptr, m_physical_ptr.get(), user_region.size,
                            attributes_for_user(readable, writeable, executable), NormalRAM)) {
        return EFAIL;
    }
    return ESUCCESS;
}
ErrorCode mem::UserOwnedAllocation::unmap_from_table(TableManager& manager, mem::UserRegion user_region, uSize offset) {
    VERIFY(user_region.page_aligned() && (offset % PAGE_SIZE) == 0 && (user_region.size + offset) <= m_region.size);
    if (!manager.unmap_region(user_region.start.get(), user_region.size)) {
        return EFAIL;
    }

    return ESUCCESS;
}
mem::UserOwnedAllocation::~UserOwnedAllocation() { mem::PageAllocator::the().free_region(m_region.start); }
expected<bek::shared_ptr<mem::BackingRegion>> mem::UserOwnedAllocation::clone_for_fork(
    UserspaceRegion& current_region) {
    if (!(current_region.permissions & MemoryOperation::Write)) {
        // We don't need to write to this region! So just copy reference (nice).
        return bek::shared_ptr<mem::BackingRegion>{this};
    } else {
        // May need to write. In future, Copy-on-Write!
        // TODO: Copy on Write.
        auto new_backing_region = EXPECTED_TRY(create_contiguous(m_region.size / PAGE_SIZE));
        bek::memcopy(new_backing_region->m_region.start.ptr, m_region.start.ptr, m_region.size);
        return new_backing_region;
    }
}
