/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
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

#ifndef BEKOS_ARCH_A64_KERNEL_ENTRY_H
#define BEKOS_ARCH_A64_KERNEL_ENTRY_H

#define STACK_REGISTER_HEADER_SZ 272

#ifdef __ASSEMBLER__
// clang-format off

.macro store_regs
    sub	sp, sp, #STACK_REGISTER_HEADER_SZ
    // Store General Purpose registers
    stp x0,  x1,  [sp, #16 * 0]
    stp	x2,  x3,  [sp, #16 * 1]
    stp	x4,  x5,  [sp, #16 * 2]
    stp	x6,  x7,  [sp, #16 * 3]
    stp	x8,  x9,  [sp, #16 * 4]
    stp	x10, x11, [sp, #16 * 5]
    stp	x12, x13, [sp, #16 * 6]
    stp	x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]

    // Store x30 and el0's stack pointer (just in case...)
    mrs x21, SP_EL0
    stp x30, x21, [sp, #16 * 15]

    // Store the return registers - for nested interrupts
    mrs x21, SPSR_EL1
    mrs x22, ELR_EL1
    stp x21, x22, [sp, #16 * 16]
.endm

.macro restore_regs
    // Restore exception regs
    ldp x0, x1, [sp, #16 * 16]
    msr SPSR_EL1, x0
    msr ELR_EL1, x1

    // Restore x30 and el0's stack pointer (just in case...)
    ldp x30, x0, [sp, #16 * 15]
    msr SP_EL0, x0

    // Restore general regs
    ldp	x0,  x1,  [sp, #16 * 0]
    ldp	x2,  x3,  [sp, #16 * 1]
    ldp	x4,  x5,  [sp, #16 * 2]
    ldp	x6,  x7,  [sp, #16 * 3]
    ldp	x8,  x9,  [sp, #16 * 4]
    ldp	x10, x11, [sp, #16 * 5]
    ldp	x12, x13, [sp, #16 * 6]
    ldp	x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]
    add sp, sp, #STACK_REGISTER_HEADER_SZ
.endm


// Define a macro so as to avoid branching/affecting lr.
.macro inline_disable_interrupts
    msr daifset, #2
.endm

// Define a macro so as to avoid branching/affecting lr.
.macro inline_enable_interrupts
    msr daifclr, #2
.endm

// clang-format on
#endif

#endif  // BEKOS_ARCH_A64_KERNEL_ENTRY_H