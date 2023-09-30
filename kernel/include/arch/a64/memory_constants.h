/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_MEMORY_CONSTANTS_H
#define BEKOS_MEMORY_CONSTANTS_H

// The start of virtual addresses (assuming 48-bit VAs)
#define VA_START 0xFFFF000000000000

// Where the kernel is mapped. The kernel will be linked at this address.
// This leaves 128TB for identity mapping.
// TODO: Synchronise between this and the linker script.
#define KERNEL_VBASE 0xFFFF800000000000

// Offset for the identity mapping of memory etc.
#define VA_IDENT_OFFSET VA_START

#define SIZE_2M (2ul << 20)

#define PAGE_SHIFT 12
#define PAGE_SIZE 4096

#endif  // BEKOS_MEMORY_CONSTANTS_H
