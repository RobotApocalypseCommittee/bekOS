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

#include <printf.h>
#include "filesystem/mbr.h"

struct raw_mbr_partition {
    u8 boot_flag;
    u8 chs_begin[3];
    u8 type_code;
    u8 chs_end[3];
    u32 lba_begin;
    u32 sector_count;
} __attribute__((packed));

MBRPartitions readMBR(BlockDevice &device, uSize block_scale) {
    MBRPartitions partitions;
    raw_mbr_partition mbrData[4];
    device.readBytes(0x1CE, &mbrData, sizeof(mbrData));
    for (int i = 0; i < 4; i++) {
        partitions.partitions[i] = {.start = mbrData[i].lba_begin, .size = mbrData[i].sector_count, .type = PART_FREE};
        switch (mbrData[i].type_code) {
            case 0xB:
            case 0xC:
                partitions.partitions[i].type = PART_FAT32;
                break;
            default:
                partitions.partitions[i].type = PART_FREE;
                break;
        }
    }
    return partitions;
}
