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

#ifndef BEKOS_MM_H
#define BEKOS_MM_H

// MEMORY AMOUNTS

// 3 * (512 entries of 8 bytes = 4kB)

#define PAGE_TABLES_SIZE 3 * (1 << 12)
#define BOOT_PTABLES_SIZE 2 * PAGE_TABLES_SIZE

#define PAGE_ENTRY_COUNT 512
#define PAGE_SHIFT 12U
#define PAGE_SIZE (1U << PAGE_SHIFT)

// ARM MMU FLAGS AND STUFF

// Link to next table in chain
#define MM_TYPE_TABLE 0b11
// References a physical block o' memory
#define MM_TYPE_BLOCK 0b01
// Reference a page(in table 3)
#define MM_TYPE_PAGE 0b11

// The translation levels
// VA [47:39]
#define LEVEL_0_SHIFT 39
// VA [38:30]
#define LEVEL_1_SHIFT 30
// VA [29:21]
#define LEVEL_2_SHIFT 21
#define LEVEL_2_SIZE 1 << LEVEL_2_SHIFT
#define LEVEL_3_SHIFT 12

// IMPORTANT things
#define MM_ACCESS			(0x1 << 10)
#define MM_ACCESS_PERMISSION		(0x01 << 6)

/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *			n	MAIR
 *   DEVICE_nGnRnE	000	00000000
 *   NORMAL_NC		001	01000100
 */
// Indexes in MAIR attributes
#define MT_DEVICE_nGnRnE 		0x0
#define MT_NORMAL_NC			0x1
// Contents of mair attributes

// A non-Gathering, non-Reordering, No Early write acknowledgement device
// Cos this isnt actually ram, but peripherals, we need to tell the processor
// Not to do anything sneaky
#define MT_DEVICE_nGnRnE_FLAGS		0x00
#define MT_NORMAL_NC_FLAGS  		0x44
#define MAIR_VALUE			(MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | (MT_NORMAL_NC_FLAGS << (8 * MT_NORMAL_NC))

#define MMU_FLAGS	 		(MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS)
#define MMU_DEVICE_FLAGS		(MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)



#ifndef __ASSEMBLER__
#include "hardtypes.h"

extern "C"
void memzero(unsigned long src, unsigned long n);

extern "C"
void set_usertable(u64 usertable);


#endif

#endif //BEKOS_MM_H
