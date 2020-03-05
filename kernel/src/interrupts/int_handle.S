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

.section ".text.vec"

.macro complain_unknown_interrupt
    store_regs
    // Arguments
    mrs	x0, esr_el1
    mrs	x1, elr_el1
    bl show_unknown_int_complaint
1:  wfe
    b 1b
.endm

.macro handle_basic_interrupt
    store_regs
    // Arguments
    mrs	x0, esr_el1
    mrs	x1, elr_el1
    bl handle_interrupt
    restore_regs
    eret
.endm


.macro store_regs
    sub	sp, sp, #176
    // Store General Purpose registers
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp x16, x17, [sp, #16 * 8]
	stp x18, x30, [sp, #16 * 9]

	// Store the return registers - for nested interrupts
	// Read SPSR_EL1 and ELR_EL1 into GP registers
    mrs x0, SPSR_EL1
    mrs x1, ELR_EL1
    // Stack SPSR_EL1 and ELR_EL1
    stp x0, x1, [sp, #16 * 10]
.endm

.macro restore_regs
    // Restore exception regs
    LDP x0, x1, [sp, #16 * 10]
    MSR SPSR_EL1, x0
    MSR ELR_EL1, x1
    // Restore general regs
    ldp x0, x1, [sp, #16 * 0]
    ldp x2, x3, [sp, #16 * 1]
    ldp x4, x5, [sp, #16 * 2]
    ldp x6, x7, [sp, #16 * 3]
    ldp x8, x9, [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x30, [sp, #16 * 9]
    add sp, sp, #176
.endm

.macro vector_entry branchlabel
.align 7
b \branchlabel
.endm

// VBAR has reserved 0 bottom 11 bits
.align 11
.globl irq_vectors
irq_vectors:
    vector_entry el1_s0_sync
    vector_entry el1_s0_irq
    vector_entry el1_s0_fiq
    vector_entry el1_s0_err

    vector_entry el1_s1_sync
    vector_entry el1_s1_irq
    vector_entry el1_s1_fiq
    vector_entry el1_s1_err

    vector_entry el0_64_sync
    vector_entry el0_64_irq
    vector_entry el0_64_fiq
    vector_entry el0_64_err

    vector_entry el0_32_sync
    vector_entry el0_32_irq
    vector_entry el0_32_fiq
    vector_entry el0_32_err

el1_s0_sync:
    complain_unknown_interrupt
el1_s0_irq:
    complain_unknown_interrupt
el1_s0_fiq:
    complain_unknown_interrupt
el1_s0_err:
    complain_unknown_interrupt
el1_s1_sync:
    complain_unknown_interrupt
el1_s1_irq:
    handle_basic_interrupt
el1_s1_fiq:
    complain_unknown_interrupt
el1_s1_err:
    complain_unknown_interrupt
el0_64_sync:
    complain_unknown_interrupt
el0_64_irq:
    handle_basic_interrupt
el0_64_fiq:
    complain_unknown_interrupt
el0_64_err:
    complain_unknown_interrupt
el0_32_sync:
    complain_unknown_interrupt
el0_32_irq:
    complain_unknown_interrupt
el0_32_fiq:
    complain_unknown_interrupt
el0_32_err:
    complain_unknown_interrupt
