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

#include "mm/space_manager.h"

#include "library/debug.h"
#include "mm.h"
#include "mm/page_allocator.h"

using DBG = DebugScope<"SpaceManager", true>;

constexpr inline uPtr virt_addr_start = 0x0000000000500000;

expected<SpaceManager> SpaceManager::create() { return SpaceManager{TableManager::create_user_manager()}; }
bool SpaceManager::check_region(uPtr location, uSize size, MemoryOperation operation) {
    mem::UserRegion check_region{location, size};
    for (auto& region : m_regions) {
        if (region.user_region.contains(check_region)) {
            return (region.permissions & operation) == operation;
        }
    }
    return false;
}
uPtr SpaceManager::raw_root_ptr() const {
    auto opt_addr = mem::kernel_virt_to_phys(m_tables.get_root_table());
    VERIFY(opt_addr);
    return opt_addr->get();
}

ErrorCode SpaceManager::deallocate_userspace_region(uPtr location, uSize size) {
    for (auto& region : m_regions) {
        // TODO: support splitting regions.
        if (region.user_region.start.get() == location && region.user_region.size == size) {
            // LOCK?
            auto res = region.backing->unmap_from_table(m_tables, region.user_region, 0);
            if (res != ESUCCESS) return res;

            auto to_deallocate = m_regions.extract(region);
            return ESUCCESS;
        }
    }
    return EINVAL;
}
expected<mem::UserRegion> SpaceManager::place_region(bek::optional<uPtr> location, MemoryOperation allowed_operations,
                                                     bek::string name, bek::shared_ptr<mem::BackingRegion> region) {
    uSize pages = bek::ceil_div(region->size(), (uSize)PAGE_SIZE);
    // FIXME: Overflow checks.

    uPtr actual_location = virt_addr_start;
    if (location) {
        actual_location = *location;
    } else if (m_regions.size() != 0) {
        actual_location = bek::align_up((m_regions.back().user_region.end().ptr), (uPtr)PAGE_SIZE);
    }
    if (actual_location + region->size() > USER_ADDR_MAX) {
        return EINVAL;
    }

    mem::UserRegion desired_region{actual_location, pages * PAGE_SIZE};
    uSize i;  // Insertion position
    for (i = 0; i < m_regions.size(); i++) {
        if (m_regions[i].user_region.overlaps(desired_region)) return {EADDRINUSE};

        if (m_regions[i].user_region.start >= desired_region.end()) {
            // Insertion point identified.
            break;
        }
    }

    // Next, try to map it.
    if (auto res = region->map_into_table(m_tables, desired_region, 0,
                                          (allowed_operations & MemoryOperation::Read) != MemoryOperation::None,
                                          (allowed_operations & MemoryOperation::Write) != MemoryOperation::None,
                                          (allowed_operations & MemoryOperation::Execute) != MemoryOperation::None);
        res != ESUCCESS) {
        return res;
    }

    if (i == m_regions.size()) {
        m_regions.push_back(UserspaceRegion{desired_region, bek::move(region), bek::move(name), allowed_operations});
    } else {
        m_regions.insert(i, UserspaceRegion{desired_region, bek::move(region), bek::move(name), allowed_operations});
    }
    return desired_region;
}
void SpaceManager::debug_print() const {
    DBG::dbgln("--- User Address Space ---"_sv);
    for (auto& region : m_regions) {
        DBG::dbgln("{} {} {}"_sv, region.user_region.start, region.user_region.end(), region.name.view());
    }
}
ErrorCode SpaceManager::deallocate_userspace_region(const bek::shared_ptr<mem::BackingRegion>& region) {
    bool found = false;
    do {
        found = false;
        for (auto& user_region : m_regions) {
            if (user_region.backing == region) {
                m_regions.extract(user_region);
                found = true;
                break;
            }
        }
    } while (found);
    return ESUCCESS;
}

expected<bek::shared_ptr<mem::UserOwnedAllocation>> SpaceManager::allocate_placed_region(
    mem::UserRegion region, MemoryOperation allowed_operations, bek::str_view name) {
    VERIFY(region.page_aligned());
    auto allocation = EXPECTED_TRY(mem::UserOwnedAllocation::create_contiguous(region.size / PAGE_SIZE));
    EXPECTED_TRY(place_region(region.start.ptr, allowed_operations, bek::string{name}, allocation));
    return allocation;
}
