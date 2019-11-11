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

#ifndef BEKOS_HDEVICE_H
#define BEKOS_HDEVICE_H


class hdevice {
public:
    virtual unsigned int get_sector_size() = 0;

    virtual bool supports_writes() = 0;

    virtual int read(unsigned long start, void* buffer, unsigned long length) = 0;

    virtual int write(unsigned long start, void* buffer, unsigned long length) = 0;
};


#endif //BEKOS_HDEVICE_H
