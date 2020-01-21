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

#ifndef BEKOS_MBR_H
#define BEKOS_MBR_H

#include <stddef.h>
#include "partition.h"


enum PartitionType: int {
    PART_FREE = 0x0,
    PART_FAT32 = 0x1,
};

struct MBR_partition {
    size_t start;
    size_t size;
    PartitionType type;
};

class master_boot_record {
public:
    explicit master_boot_record(void* buf, hdevice* device);
    // No copying is allowed
    master_boot_record(const master_boot_record&) = delete;
    void operator=(const master_boot_record&) = delete;

    MBR_partition* get_partition_info(int id);
    partition* get_partition(int id);

    virtual ~master_boot_record();

private:
    hdevice* m_device;
    MBR_partition partitions[4];
    partition* d_partitions[4];

};

#endif //BEKOS_MBR_H
