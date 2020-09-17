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

#ifndef BEKOS_DEVICE_H
#define BEKOS_DEVICE_H

class BlockDevice {
public:
    virtual unsigned long block_size() = 0;
    virtual bool supports_writes() = 0;

    bool readBytes(unsigned long start, void* buffer, unsigned long count);
    bool writeBytes(unsigned long start, const void* buffer, unsigned long count);

    virtual bool readBlock(unsigned long index, void* buffer, unsigned long offset, unsigned long count) = 0;
    virtual bool writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) = 0;

};


#endif //BEKOS_DEVICE_H
