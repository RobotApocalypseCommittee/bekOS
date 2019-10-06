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

#ifndef BEKOS_UTILS_H
#define BEKOS_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "page_mapping.h"

void memcpy(void * src, void* dest, size_t length);
extern "C"
void * memset ( void * ptr, int value, size_t num );

uint32_t byte_swap32(uint32_t swapee);

unsigned long phys_to_virt(unsigned long physical_address);

unsigned long virt_to_phys(unsigned long virtual_address);

#endif //BEKOS_UTILS_H
