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

	// Disable coprocessor traps
    mov     x0, #0x33FF
	msr		CPTR_EL2, x0
	msr		HSTR_EL2, xzr
	mov		x0, #3 << 20
	msr 	CPACR_EL1, x0 // Enable FP and SIMD

	// Set lower levels to 64 bit
	mov		x0, #(1 << 31)
	msr		HCR_EL2, x0

    // Disable MMU, and set system control
    ldr		x0, =SCTLR_VALUE_MMU_DISABLED
	msr	    SCTLR_EL1, x0

    // EL3 Configuration
	//ldr	x0, =SCR_VALUE
	//msr	scr_el3, x0

    // Saved status for EL3/2 -> El1
	ldr		x0, =SPSR_VALUE
	msr		spsr_el2, x0

    // Store the address of the next code to run in the link register
    // This register is used when returning from exception
	adr		x0, master
	msr		elr_el2, x0
    // Return from 'exception'
	eret
master:
    // Clear bss section to 0, get start and length from linker
    // Load our offset into a register

    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    sub     x1, x1, x0      // Calculate size
    and     x0, x0, #0x0000FFFFFFFFFFFF // Make unmapped
    bl __memzero

    // set stack to start of code cos it grows downwards
    ldr     x0, =__start
    and     x0, x0, #0x0000FFFFFFFFFFFF // Make unmapped
    mov     sp, x0

	bl kernel_boot
	// x0 is where we branch to
	mov x1, x0

	// Now memory is mapped, we can do this properly
	ldr     x0, =__start
    mov     sp, x0

	mrs     x0, sctlr_el1
	orr     x0, x0, #1 // Enable bit
	msr     sctlr_el1, x0

	isb

	blr x1 // Branch to new address - this instruction must be mapped.

    // stop this core
1:  wfe
    b 1b

__memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt __memzero
    ret