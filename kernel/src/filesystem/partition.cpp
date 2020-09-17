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

Partition::Partition(BlockDevice *source, unsigned long start_index, unsigned long size_n) : source(source),
                                                                                             start_block(start_index),
                                                                                             partition_size_blocks(
                                                                                                     size_n) {}

bool Partition::supports_writes() {
    return source->supports_writes();
}

unsigned long Partition::block_size() {
    return source->block_size();
}

bool Partition::readBlock(unsigned long index, void *buffer, unsigned long offset, unsigned long count) {
    if (index >= partition_size_blocks) return false;

    return readBlock(index + start_block, buffer, offset, count);
}

bool Partition::writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) {
    if (index >= partition_size_blocks) return false;

    return writeBlock(index + start_block, buffer, offset, count);
}
