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

struct SavedRegs {
    // These are the registers that are assumed not to change...
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
    u64 fp;
    u64 sp;
    u64 pc;
    u64 el0_sp;
    // TODO: SIMD / Floating Point

    // Creates state setup for executing a NORETURN fn, passing in arg.
    static SavedRegs create_for_kernel(void (*fn)(void*), void* arg, void* stack_top, uPtr user_stack_ptr = 0);
};

#endif  // BEKOS_A64_SAVED_REGISTERS_H
