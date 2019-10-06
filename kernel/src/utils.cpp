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
#include "utils.h"

void memcpy(void* src, void* dest, size_t length) {
    uint8_t* bsrc = (uint8_t*) src;
    uint8_t* bdest = (uint8_t*) dest;
    for (size_t i = 0; i < length; i++) {
        bdest[i] = bsrc[i];
    }
}

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

void* memset(void* ptr, int value, size_t num) {
    uint8_t* bptr = (uint8_t*) ptr;
    for (size_t i = 0; i < num; i++) {
        bptr[i] = value;
    }
}
