/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_MBR_H
#define BEKOS_MBR_H

#include "filesystem/partition.h"
#include "library/types.h"

constexpr inline uSize MBR_OFFSET = 0x1BE;

struct [[gnu::packed]] RawMbrPartition {
    u8 boot_flag;
    u8 chs_begin[3];
    u8 type_code;
    u8 chs_end[3];
    u32 lba_begin;
    u32 sector_count;
};

constexpr blk::PartitionFsKind kind_from_code(u8 type_code) {
    switch (type_code) {
        case 0xB:
        case 0xC:
            return blk::PartitionFsKind::Fat;
        default:
            return blk::PartitionFsKind::Unrecognised;
    }
}

#endif //BEKOS_MBR_H
