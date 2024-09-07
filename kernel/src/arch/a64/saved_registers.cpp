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

extern "C" [[noreturn]] void kfunction_stub_a64();

SavedRegs SavedRegs::create_for_kernel(void (*fn)(void*), void* arg, void* stack_top, uPtr user_stack_ptr) {
    SavedRegs regs{};
    regs.x19 = reinterpret_cast<u64>(fn);
    regs.x20 = reinterpret_cast<u64>(arg);
    regs.sp = reinterpret_cast<u64>(stack_top);
    regs.pc = reinterpret_cast<u64>(kfunction_stub_a64);
    regs.el0_sp = user_stack_ptr;
    return regs;
}
