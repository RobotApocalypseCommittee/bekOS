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

    mrs     x0, CurrentEL
    and     x0, x0, #12 // clear reserved bits

    // running at EL3?
    cmp     x0, #12
    bne     el2
    // should never be executed, just for completeness
    mov     x2, #0x5b1
    msr     scr_el3, x2
    mov     x2, #0x3c9
    msr     spsr_el3, x2
    adr     x2, el2
    msr     elr_el3, x2
    eret

    // running at EL2?
el2:  cmp     x0, #4
    beq     el1
    msr     sp_el1, x1
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // change execution level to EL1
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, el1
    msr     elr_el2, x2
    eret
el1:
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
	adr     x1, kernel_main
	orr      x1, x1, #0xFFFF000000000000

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