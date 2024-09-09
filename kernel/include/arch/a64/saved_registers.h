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

#ifndef BEKOS_A64_SAVED_REGISTERS_H
#define BEKOS_A64_SAVED_REGISTERS_H

#include "bek/types.h"
#include "kernel_entry.h"

// When we enter an interreupt, we save registers onto the top of the stack.
// This structure represents those registers.
struct alignas(16) InterruptContext {
    u64 x[31];  // Includes x30, return address.
    u64 sp_el0;
    u64 spsr_el1;
    u64 elr_el1;

    void set_return_value(u64 value) { x[0] = value; }
};

static_assert(sizeof(InterruptContext) == STACK_REGISTER_HEADER_SZ);

// The Registers which need to be preserved as if calling a function.
// This is used for context switching, and by process first entry.
struct SavedRegisters {
    u64 x19;
    u64 x20;
    u64 x21;
    u64 x22;
    u64 x23;
    u64 x24;
    u64 x25;
    u64 x26;
    u64 x27;
    u64 x28;
    u64 fp;  // Otherwise known as x29.
    u64 lr;  // Otherwise known as x30/or pc!
    u64 sp;
    u64 el0_sp;
    // TODO: SIMD / Floating Point

    static SavedRegisters create_for_kernel_task(void (*task)(void*), void* arg, void* kernel_stack_top);
    static SavedRegisters create_for_user_execute(uPtr user_entry, void* kernel_stack_top, uPtr user_stack);
    static SavedRegisters create_for_return_from_fork(const InterruptContext& ctx, void* kernel_stack_top,
                                                      uPtr current_user_stack);
};

#endif  // BEKOS_A64_SAVED_REGISTERS_H
