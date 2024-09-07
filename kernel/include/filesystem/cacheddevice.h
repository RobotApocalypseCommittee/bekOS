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

#ifndef BEKOS_CACHEDDEVICE_H
#define BEKOS_CACHEDDEVICE_H

#include "block_device.h"
#include "blockcache.h"

class CachedDevice final: public BlockDevice {
public:
    explicit CachedDevice(BlockCache& cache, BlockDevice& device);

    bool supports_writes() override;

    unsigned long block_size() override;

    bool readBlock(unsigned long index, void *buffer, unsigned long offset, unsigned long count) override;

    bool writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) override;

private:
    BlockDevice& m_device;
    BlockIndexer m_cache;
};

#endif //BEKOS_CACHEDDEVICE_H
