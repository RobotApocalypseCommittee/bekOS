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

#ifndef BEKOS_SPACE_MANAGER_H
#define BEKOS_SPACE_MANAGER_H

#include "arch/a64/translation_tables.h"
#include "backing_region.h"
#include "bek/bitwise_enum.h"
#include "bek/str.h"
#include "bek/vector.h"
#include "library/kernel_error.h"

enum class MemoryOperation {
    None = 0x0,
    Read = 0x1,
    Write = 0x2,
    Execute = 0x4,
};

template <>
constexpr inline bool bek_is_bitwise_enum<MemoryOperation> = true;

/// Short for User Address Space Region.
struct UserspaceRegion {
    mem::UserRegion user_region;
    bek::shared_ptr<mem::BackingRegion> backing;
    bek::string name;
    MemoryOperation permissions;
};

class SpaceManager {
public:
    static expected<SpaceManager> create();

    expected<mem::UserRegion> place_region(bek::optional<uPtr> location, MemoryOperation allowed_operations,
                                           bek::string name, bek::shared_ptr<mem::BackingRegion> region);
    bool check_region(uPtr location, uSize size, MemoryOperation operation);
    ErrorCode deallocate_userspace_region(uPtr location, uSize size);
    ErrorCode deallocate_userspace_region(const bek::shared_ptr<mem::BackingRegion>& region);

    expected<bek::shared_ptr<mem::UserOwnedAllocation>> allocate_placed_region(mem::UserRegion region,
                                                                               MemoryOperation allowed_operations,
                                                                               bek::str_view name);
    expected<mem::UserRegion> allocate_flexible_region(uSize size, MemoryOperation allowed_operations,
                                                       bek::str_view name, bek::optional<mem::UserPtr> hint);

    void debug_print() const;
    uPtr raw_root_ptr() const;

    SpaceManager(const SpaceManager&) = delete;
    SpaceManager& operator=(const SpaceManager&) = delete;

    SpaceManager(SpaceManager&&) = default;
    SpaceManager& operator=(SpaceManager&&) = default;

private:
    explicit SpaceManager(TableManager manager) : m_tables(bek::move(manager)) {}

    bek::vector<UserspaceRegion> m_regions{};
    TableManager m_tables;
};

#endif  // BEKOS_SPACE_MANAGER_H
