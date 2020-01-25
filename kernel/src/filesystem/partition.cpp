/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "filesystem/partition.h"

partition::partition(hdevice* source, unsigned long startSector, unsigned long sectorSize, unsigned long partitionSize)
        : source(source), start_sector(startSector), sector_size(sectorSize), partition_size(partitionSize) {}

unsigned int partition::get_sector_size() {
    return sector_size;
}

bool partition::supports_writes() {
    return source->supports_writes();
}

int partition::read(unsigned long start, void* buffer, unsigned long length) {
    // Check alignment and length
    if (start % sector_size || start > partition_size || length % sector_size || start + length > partition_size) {
        return -1;
    }
    // Lets go
    unsigned long actual_start = start + (start_sector*sector_size);
    source->read(actual_start, buffer, length);
    return 0;
}

int partition::write(unsigned long start, void* buffer, unsigned long length) {
    // Check alignment and length
    if (start % sector_size || start > partition_size || length % sector_size || start + length > partition_size) {
        return -1;
    }
    // Lets go
    unsigned long actual_start = start + (start_sector*sector_size);
    source->write(actual_start, buffer, length);
    return 0;
}
