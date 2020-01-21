/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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

#include <stdint.h>
#include <printf.h>
#include "filesystem/mbr.h"

struct _mbr_partition {
    uint8_t boot_flag;
    uint8_t chs_begin[3];
    uint8_t type_code;
    uint8_t chs_end[3];
    uint32_t lba_begin;
    uint32_t sector_count;
} __attribute__((packed));

master_boot_record::master_boot_record(void* buf, hdevice* device): m_device(device), d_partitions{nullptr, nullptr,
                                                                                                    nullptr, nullptr} {
    _mbr_partition* parts = reinterpret_cast<_mbr_partition*>(
            reinterpret_cast<uint8_t*>(buf) + 446);
    for (int i = 0; i < 4; i++){
        partitions[i].start = parts->lba_begin;
        // TODO: Dont hardcode
        partitions[i].size = parts->sector_count*512;
        // TODO: Generalise
        switch(parts->type_code) {
            case 0xB:
            case 0xC:
                partitions[i].type = PART_FAT32;
                break;
            default:
                partitions[i].type = PART_FREE;
                break;
        }
        printf("Partition at %lX, is %lX sectors long, and is type %X\n", partitions[i].start, parts->sector_count, parts->type_code);
        parts += 1;
    }
}

MBR_partition* master_boot_record::get_partition_info(int id) {
    if (id < 4 && id >= 0) {
        return &partitions[id];
    } else {
        // TODO: Error
        printf("Partition %d doesnt exist!", id);
        return nullptr;
    }
}

master_boot_record::~master_boot_record() {
    for (auto & d_partition : d_partitions) {
        delete d_partition;
    }

}

partition* master_boot_record::get_partition(int id) {
    auto p = get_partition_info(id);
    if (p->type != PART_FREE) {
        if (d_partitions[id] == nullptr) {
            d_partitions[id] = new partition(m_device, p->start, 512, p->size);
        }
        return d_partitions[id];
    } else {
        printf("Cannot get non existent partition.");
        return nullptr;
    }
}
