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

#include <memory_locations.h>
#include <kstring.h>


#include "utils.h"


uint32_t byte_swap32(uint32_t swapee) {
    return ((swapee & 0xFF000000) >> 24) |
            ((swapee & 0x00FF0000) >> 8) |
            ((swapee & 0x0000FF00) << 8) |
            ((swapee & 0x000000FF) << 24);
}

unsigned long phys_to_virt(unsigned long physical_address) {
    return physical_address + VA_START;
}

unsigned long virt_to_phys(unsigned long virtual_address) {
    return virtual_address - VA_START;
}



unsigned int read_u32_LE(void* data, unsigned int offset) {
    uint8_t* dat = reinterpret_cast<uint8_t*>(data);
    return dat[offset] | dat[offset+1] << 8 | dat[offset+2] << 16 | dat[offset+3] << 24;
}

unsigned short read_u16_LE(void* data, unsigned int offset) {
    uint8_t* dat = reinterpret_cast<uint8_t*>(data);
    return dat[offset] | dat[offset+1] << 8;

}

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
}
