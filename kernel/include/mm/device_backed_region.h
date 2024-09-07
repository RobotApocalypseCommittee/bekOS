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

#ifndef BEKOS_DEVICE_BACKED_REGION_H
#define BEKOS_DEVICE_BACKED_REGION_H

#include "backing_region.h"
#include "peripherals/device.h"

class DeviceBackedRegion : public mem::BackingRegion {
public:
    explicit DeviceBackedRegion(mem::PhysicalRegion region) : m_region(region) {}
    uSize size() const override { return m_region.size; }
    ErrorCode map_into_table(TableManager& manager, mem::UserRegion user_region, uSize offset, bool readable,
                             bool writable, bool executable) override;
    ErrorCode unmap_from_table(TableManager& manager, mem::UserRegion user_region, uSize offset) override;

private:
    mem::PhysicalRegion m_region;
};

#endif  // BEKOS_DEVICE_BACKED_REGION_H
