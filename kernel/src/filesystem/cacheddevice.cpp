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

#include "filesystem/cacheddevice.h"

#include <c_string.h>

CachedDevice::CachedDevice(BlockCache &cache, BlockDevice &device): m_device(device), m_cache(cache, device.block_size()) {
}

bool CachedDevice::supports_writes() {
    return m_device.supports_writes();
}


unsigned long CachedDevice::block_size() {
    return m_device.block_size();
}

bool CachedDevice::readBlock(unsigned long index, void *buffer, unsigned long offset, unsigned long count) {
    ASSERT(offset + count <= block_size());
    auto entry = m_cache.get(index);
    ASSERT(entry->data != nullptr);
    if (!entry->loaded) {
        if (!m_device.readBlock(index, entry->data, 0, block_size())) {
            return false;
        }
        entry->loaded = true;
    }
    if (buffer) memcpy((char *)entry->data + offset, buffer, count);
    return true;
}

bool CachedDevice::writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) {
    ASSERT(offset + count <= block_size());
    auto entry = m_cache.get(index);
    ASSERT(entry->data != nullptr);
    if ((!entry->loaded) && count != block_size()) {
        if (!m_device.readBlock(index, entry->data, 0, block_size())) {
            return false;
        }
        entry->loaded = true;
    }
    memcpy(reinterpret_cast<u8 *>(entry->data) + offset, buffer, count);
    entry->dirty = true;
    return true;
}
