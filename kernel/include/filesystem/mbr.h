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

#include "library/types.h"
#include "partition.h"


enum PartitionType: int {
    PART_FREE = 0x0,
    PART_FAT32 = 0x1,
};

struct MBRPartitions {
    struct {
        uSize start;
        uSize size;
        PartitionType type;
    } partitions[4];
};

MBRPartitions readMBR(BlockDevice &device, uSize block_scale = 1);

#endif //BEKOS_MBR_H
