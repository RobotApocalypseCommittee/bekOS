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

#ifndef BEKOS_BACKING_REGION_H
#define BEKOS_BACKING_REGION_H

#include "arch/a64/translation_tables.h"
#include "bek/own_ptr.h"
#include "library/intrusive_shared_ptr.h"
#include "library/kernel_error.h"

class UserspaceRegion;

namespace mem {

/// Virtual Class for a region of memory which backs a region of userspace address space.
class BackingRegion: public bek::RefCounted<BackingRegion> {
public:
    virtual uSize size() const = 0;
    /**
     * Map backing region into process address space.
     * If offset != 0 or user_region.size() < size() a subset of the region will be mapped.
     * @param manager The manager of the userspace address space.
     * @param user_region Region to map. Must be page-aligned.
     * @param offset Offset into backing region to map. Must be page-aligned.
     * @return ESUCCESS if successful, otherwise relevant ErrorCode.
     */
    virtual ErrorCode map_into_table(TableManager& manager, UserRegion user_region, uSize offset,
                                     bool readable, bool writable, bool executable) = 0;
    /**
     * Unmaps backing region which was mapped into `manager` by map_into_table.
     * Arguments must be same as were provided to map_into_table, or behaviour undefined.
     * @param manager
     * @param user_region
     * @param offset
     * @return
     */
    virtual ErrorCode unmap_from_table(TableManager& manager, UserRegion user_region,
                                       uSize offset) = 0;
    virtual ~BackingRegion()                         = default;

    virtual expected<bek::shared_ptr<BackingRegion>> clone_for_fork(UserspaceRegion& current_region) = 0;

    BackingRegion()                                = default;
    BackingRegion(const BackingRegion&)            = delete;
    BackingRegion& operator=(const BackingRegion&) = delete;
    BackingRegion(BackingRegion&&)                 = default;
    BackingRegion& operator=(BackingRegion&&)      = default;
};

class UserOwnedAllocation : public BackingRegion {
public:

    static expected<bek::shared_ptr<UserOwnedAllocation>> create_contiguous(uSize pages);

public:
    constexpr mem::VirtualRegion kernel_mapped_region() const {
        return m_region;
    }
    uSize size() const override { return m_region.size; }
    ErrorCode map_into_table(TableManager& manager, UserRegion user_region, uSize offset,
                             bool readable, bool writeable, bool executable) override;
    ErrorCode unmap_from_table(TableManager& manager, UserRegion user_region,
                               uSize offset) override;
    expected<bek::shared_ptr<BackingRegion>> clone_for_fork(UserspaceRegion& current_region) override;

    UserOwnedAllocation(UserOwnedAllocation&&) = default;
    ~UserOwnedAllocation() override;

private:
    explicit UserOwnedAllocation(VirtualRegion region, PhysicalPtr physical_ptr)
        : m_region{region}, m_physical_ptr(physical_ptr) {
        VERIFY(m_region.page_aligned());
        VERIFY(m_physical_ptr.page_offset() == 0);
    }

    mem::VirtualRegion m_region;
    mem::PhysicalPtr m_physical_ptr;
};

}  // namespace mem

#endif  // BEKOS_BACKING_REGION_H
