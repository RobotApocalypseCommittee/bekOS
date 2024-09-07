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

#include "arch/a64/sysreg_constants.h"
#include "arch/a64/asm_defines.h"

.macro adrl, dst, sym
    adrp    \dst, \sym
    add     \dst, \dst, :lo12:\sym
.endm

// To ensure it's at the start.
// This is not a function.
.section ".text.boot"
.globl _head
_head:
    b   _start  // Is there a need for a header
    nop
    .quad   0
    .quad   (__kernel_sz)
    .quad   (1 << 1)  // 4KB Pages.
    .quad   0
    .quad   0
    .quad   0
    .word   0x644d5241
    .word   0  // Should not be 0?

// Kernel Entry start point
ASM_FUNCTION_BEGIN(_start)
    mov     x19, x0  // Device Tree Address
    mrs     x1, mpidr_el1 
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0 so stop
1:  wfe 
    b       1b
2:  // cpu id == 0
    mrs     x0, CurrentEL
    and     x0, x0, #12  // clear reserved bits
    // running at EL3?
    cmp     x0, #12
    bne     el2 // should never be executed, just for completeness
    ldr     x2, =SCR_VALUE
    msr     scr_el3, x2
    ldr     x2, =SPSR_EL3_VALUE
    msr     spsr_el3, x2
    adr     x2, el2
    msr     elr_el3, x2
    eret
    // running at EL2?
el2:
    cmp     x0, #4
    beq     el1
    msr     sp_el1, x1
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    // disable coprocessor traps
    mov     x0, #0x33FF
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mov     x0, #(3 << 20) 
    msr     cpacr_el1, x0
    // enable AArch64 in EL1
    ldr     x0, =HCR_VALUE
    msr     hcr_el2, x0 
    mrs     x0, hcr_el2
    // Setup SCTLR access
    ldr     x2, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x2
    // change execution level to EL1
    ldr     x2, =SPSR_EL2_VALUE
    msr     spsr_el2, x2 
    adr     x2, el1
    msr     elr_el2, x2
    eret
el1:
    // Clear bss section to 0, get start and length from linker
    // Load our offset into a register
    adrl    x0,  __bss_start 
    adrl    x1, __bss_end 
    sub     x1, x1, x0  // Calculate size
    bl      __memzero

    // set stack to start of code cos it grows downwards
    adrl    x0, __stack_top
    mov     sp, x0

    // Calling early_boot(load_address, device_tree_address)
    adrl    x0, __kernel_start
    mov     x1, x19  // Device Tree ptr
    bl      setup_early_tables 
    cbz     x0, 2f                // If success (0) move onwards.
1:  wfe 
    b       1b
2:  // Now, we need to explicitly transition to upper-half addresses.
    ldr     x1, =kernel_boot

    ldr     x0, =__stack_top
    mov     sp, x0

    ldr     x0, =SCTLR_VALUE_MMU_ENABLED 
    msr     sctlr_el1, x0 
    isb

    mov     x29, xzr  // 0-out frame pointer not to confuse debugger.
    ldr     x0, =__devtree_start 
    bfi     x0, x19, #0, #20  // Device Tree address - need to add offset.

    blr     x1  // Branch to new address
    // Should never return...
1:  wfe
    b       1b
ASM_FUNCTION_END(_start)

__memzero: 
    str     xzr, [x0], #8
    subs    x1, x1, #8 
    b.gt    __memzero 
    ret