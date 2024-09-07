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

#include "mm/device_backed_region.h"

ErrorCode DeviceBackedRegion::map_into_table(TableManager& manager, mem::UserRegion user_region, uSize offset,
                                             bool readable, bool writable, bool executable) {
    VERIFY(user_region.page_aligned() && (offset % PAGE_SIZE) == 0 && (user_region.size + offset) <= m_region.size);

    if (!manager.map_region(user_region.start.get(), m_region.start.get(), user_region.size,
                            attributes_for_user(readable, writable, executable), MMIO)) {
        return EFAIL;
    }
    return ESUCCESS;
}
ErrorCode DeviceBackedRegion::unmap_from_table(TableManager& manager, mem::UserRegion user_region, uSize offset) {
    VERIFY(user_region.page_aligned() && (offset % PAGE_SIZE) == 0 && (user_region.size + offset) <= m_region.size);
    if (!manager.unmap_region(user_region.start.get(), user_region.size)) {
        return EFAIL;
    }

    return ESUCCESS;
}
