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

#ifndef BEKOS_ARCH_PROCESS_ENTRY_H
#define BEKOS_ARCH_PROCESS_ENTRY_H

#include "bek/types.h"

struct SavedRegisters;
struct InterruptContext;

/// Switch context to next process. On return, will have switched back to previous process.
/// \pre Must switch to next user address space prior to call.
extern "C" void do_context_switch(SavedRegisters& previous, SavedRegisters& next);

/// Switches user address space.
/// \param new_address_root Physical address of root page table. Returned from SpaceManager.
extern "C" void do_switch_user_address_space(uPtr new_address_root);

extern "C" uPtr do_get_current_user_stack();

/// Assumes state provided by registers *as-if* it had been context-switched to.
/// \pre Interrupts are disabled.
/// \param kernel_sp The desired top of the kernel stack.
/// \param registers The desired context. pc will be next instruction executed after context restored.
/// \param argument If provided, pc is start of a function, will be as if first argument was provided as such.
/// \post Caller-saved registers will be 0, so registers.pc must be either start of function, or following function
/// call.
extern "C" [[noreturn]] void do_assume_process_state(SavedRegisters& registers, u64 argument);

#endif  // BEKOS_ARCH_PROCESS_ENTRY_H
