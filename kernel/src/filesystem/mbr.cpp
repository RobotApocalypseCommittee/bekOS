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

struct mbr_partition {
    uint8_t boot_flag;
    uint8_t chs_begin[3];
    uint8_t type_code;
    uint8_t chs_end[3];
    uint32_t lba_begin;
    uint32_t sector_count;
} __attribute__((packed));

master_boot_record::master_boot_record(void* buf) {
    mbr_partition* parts = reinterpret_cast<mbr_partition*>(
            reinterpret_cast<uint8_t*>(buf) + 446);
    for (int i = 0; i < 4; i++){
        partitions[i].start = parts->lba_begin;
        // TODO: Dont hardcode
        partitions[i].size = parts->sector_count*512;
        partitions[i].type_code = parts->type_code;
        printf("Partition at %lX, is %lX sectors long, and is type %X", partitions[i].start, parts->sector_count, parts->type_code);
        parts += 1;
    }
}
