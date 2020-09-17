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

#ifndef BEKOS_PARTITION_H
#define BEKOS_PARTITION_H


#include "device.h"

class Partition : public BlockDevice {
public:
    Partition(BlockDevice *source, unsigned long start_index, unsigned long size_n);

    bool supports_writes() override;

    unsigned long block_size() override;

    bool readBlock(unsigned long index, void *buffer, unsigned long offset, unsigned long count) override;

    bool writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) override;

private:
    BlockDevice *source;
    unsigned long start_block;
    unsigned long partition_size_blocks;

};


#endif //BEKOS_PARTITION_H
