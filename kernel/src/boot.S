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

// Aarch64
#include "system_registers.h"
#include "mm.h"
#include "memory_locations.h"

// To ensure it's at the start
.section ".text.boot"

// Makes it global
.globl _start
// Kernel entry point
_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0 so stop
1:  wfe
    b       1b
2:  // cpu id == 0

    // Move to EL1 - OS exec level
    // Disable MMU, and set system control
    ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	// Disable trapping of accessing in EL3 and EL2.
    MSR CPTR_EL3, XZR
    MSR CPTR_EL3, XZR
    // Disable access trapping in EL1 and EL0.
    MOV X1, #(0x3 << 20) // FPEN disables trapping to EL1.
    MSR CPACR_EL1, X1
    ISB

    // Hypervisor(EL2) state
	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

    // EL3 Configuration
	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

    // Saved status for EL3 -> El1
	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0

    // Store the address of the next code to run in the link register
    // This register is used when returning from exception
	adr	x0, master
	msr	elr_el3, x0
    // Return from 'exception'
	eret
master:
    // Clear bss section to 0, get start and length from linker
    // Load our offset into a register

    adr     x0, __hard_bss_start
    ldr     x1, =__bss_size
    bl __memzero
    // Set up tables for memory mapping
    bl memory_map

    // set stack to start of code cos it grows downwards
    ldr     x0, =__virt_start
    mov     sp, x0

    adrp	x0, __hard_page_table_start
	msr	ttbr1_el1, x0
	add     x0, x0, #PAGE_TABLES_SIZE
	msr ttbr0_el1, x0

	ldr	x0, =(TCR_VALUE)
	msr	tcr_el1, x0

	ldr	x0, =(MAIR_VALUE)
	msr	mair_el1, x0


	ldr	x2, =kernel_main

	mov	x0, #SCTLR_MMU_ENABLED
	msr	sctlr_el1, x0

	blr x2
    // stop this core
1:  wfe
    b 1b

    .macro	create_0_and_1_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, LEVEL_0_SHIFT, \tmp1, \tmp2
	create_table_entry \tbl, \virt, LEVEL_1_SHIFT, \tmp1, \tmp2
	.endm

	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
    // Shift right to get the index for *this* table to the front
	lsr	\tmp1, \virt, #\shift
    // Filter out everything above this table in the index
	and	\tmp1, \tmp1, #PAGE_ENTRY_COUNT - 1			// table index
    // Get the address of the table this entry links to(the next table)
	add	\tmp2, \tbl, #PAGE_SIZE
    // Add the flags and things for a -> next table entry
	orr	\tmp2, \tmp2, #MM_TYPE_TABLE
    // Store the entry to the apropriate place in *this* table
    // (Shift address by 3 cos each entry is 2^3 bytes)
	str	\tmp2, [\tbl, \tmp1, lsl #3]
    // Offset to link to next table to fill in
	add	\tbl, \tbl, #PAGE_SIZE					// next level table page
	.endm

	.macro	create_section_mapping, tbl, phys, start, end, flags, tmp1
    // Sections are blocks of 2MB, instead of the 4KB page size
    // Thus we can skip the final level of mapping(level 3) if we dont need high detail
    // Shift right to get the index for *this* table to the front
	lsr	\start, \start, #LEVEL_2_SHIFT
    // Filter out everything above this table in the index
	and	\start, \start, #PAGE_ENTRY_COUNT - 1			// table index
    // Do the same to the end of the mapping
	lsr	\end, \end, #LEVEL_2_SHIFT
	and	\end, \end, #PAGE_ENTRY_COUNT - 1				// table end index

    // The physical address can only be mapped with 2MB detail
	lsr	\phys, \phys, #LEVEL_2_SHIFT
    // Add the flags to the bottom bits of phys, to resemble the block entry
	mov	\tmp1, #\flags
    // We shift the phys back to make space for flags
	orr	\phys, \tmp1, \phys, lsl #LEVEL_2_SHIFT			// table entry
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry
	add	\start, \start, #1					// next entry
	add	\phys, \phys, #LEVEL_2_SIZE				// next section
	cmp	\start, \end
	b.ls	9999b
	.endm

memory_map:
	mov	x29, x30						// save return address

    // Get 'page-relative' address
	adrp	x0, __hard_page_table_start
	mov	x1, #BOOT_PTABLES_SIZE
	bl 	__memzero

	adrp	x0, __hard_page_table_start
	mov	x1, #VA_START
	create_0_and_1_entry x0, x1, x2, x3

	/* Mapping kernel and init stack*/
	mov 	x1, xzr							// start mapping from physical offset 0
	mov 	x2, #VA_START						// first virtual address
	ldr	x3, =(VA_START + PERIPHERAL_OFFSET - LEVEL_2_SIZE)		// last virtual address
	create_section_mapping x0, x1, x2, x3, MMU_FLAGS, x4

	/* Mapping device memory*/
	mov 	x1, #PERIPHERAL_OFFSET					// start mapping from device base address
	ldr 	x2, =(VA_START + PERIPHERAL_OFFSET)				// first virtual address
	ldr	x3, =(VA_START + ADDRESSABLE_MEMORY - LEVEL_2_SIZE)	// last virtual address
	create_section_mapping x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

	add x0, x0, #PAGE_SIZE

	// We can now map ttbr0 for a temporary instruction when the switch occurs
	// We map the whole of the boot sector
	mov x1, xzr
	create_0_and_1_entry x0, x1, x2, x3

	// Map section
	ldr x1, =__start // Physically beginning of boot section
	mov x2, x1 // Virtual is = cos identity mapping
	ldr x3, =__boot_end
	create_section_mapping x0, x1, x2, x3, MMU_FLAGS, x4



	mov	x30, x29						// restore return address
	ret

__memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt __memzero
    ret