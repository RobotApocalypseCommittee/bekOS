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


#include "hdevice.h"

class partition: public hdevice {
public:
    partition(hdevice* source, unsigned long startSector, unsigned long sectorSize, unsigned long partitionSize);

    unsigned int get_sector_size() override;

    bool supports_writes() override;

    int read(unsigned long start, void* buffer, unsigned long length) override;

    int write(unsigned long start, void* buffer, unsigned long length) override;

private:
    hdevice* source;
    unsigned long start_sector;
    unsigned long sector_size;
    unsigned long partition_size;

};


#endif //BEKOS_PARTITION_H
