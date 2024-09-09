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

#include "arch/a64/saved_registers.h"

// x19 = fn, x20 = arg.
extern "C" [[noreturn]] void kernel_first_entry_a64();

// x19 = user_entry.
extern "C" [[noreturn]] void user_first_entry_a64();

// x19 = user_entry.
extern "C" [[noreturn]] void ret_from_fork_a64();

SavedRegisters SavedRegisters::create_for_kernel_task(void (*task)(void*), void* arg, void* kernel_stack_top) {
    SavedRegisters regs{};
    regs.x19 = reinterpret_cast<uPtr>(task);
    regs.x20 = reinterpret_cast<uPtr>(arg);
    regs.sp = reinterpret_cast<uPtr>(kernel_stack_top);
    regs.lr = reinterpret_cast<uPtr>(kernel_first_entry_a64);
    return regs;
}

SavedRegisters SavedRegisters::create_for_user_execute(uPtr user_entry, void* kernel_stack_top, uPtr user_stack) {
    SavedRegisters regs{};
    regs.x19 = reinterpret_cast<uPtr>(user_entry);
    regs.sp = reinterpret_cast<uPtr>(kernel_stack_top);
    regs.lr = reinterpret_cast<uPtr>(user_first_entry_a64);
    regs.el0_sp = reinterpret_cast<uPtr>(user_stack);
    return regs;
}

SavedRegisters SavedRegisters::create_for_return_from_fork(const InterruptContext& ctx, void* kernel_stack_top,
                                                           uPtr current_user_stack) {
    // We need to push ctx onto kernel stack.
    uPtr kernel_stack_pushed = reinterpret_cast<uPtr>(kernel_stack_top) - sizeof(InterruptContext);
    auto& new_ctx = *reinterpret_cast<InterruptContext*>(kernel_stack_pushed);
    new_ctx = ctx;
    new_ctx.set_return_value(0);

    SavedRegisters regs{};
    regs.sp = kernel_stack_pushed;
    regs.lr = reinterpret_cast<uPtr>(ret_from_fork_a64);
    regs.el0_sp = current_user_stack;
    return regs;
}
