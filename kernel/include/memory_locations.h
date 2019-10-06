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

#ifndef BEKOS_MEMORY_LOCATIONS_H
#define BEKOS_MEMORY_LOCATIONS_H

// The start of virtual addresses in KERNEL
// All (explicit) memory accesses should be relative to this
#define VA_START 0xffff000000000000

// We have 1GB memory on Pi
#define ADDRESSABLE_MEMORY 0x40000000
#ifndef __ASSEMBLER__
extern unsigned long __virt_start, __end;

#define KERNEL_START ((unsigned long)&__virt_start - (unsigned long)VA_START)
#define KERNEL_END ((unsigned long)&__end - (unsigned long)VA_START)
#define KERNEL_SIZE (KERNEL_END - KERNEL_START)
#endif

#define PERIPHERAL_OFFSET 0x3F000000
#define PERIPHERAL_BASE (VA_START + PERIPHERAL_OFFSET)

#endif //BEKOS_MEMORY_LOCATIONS_H
