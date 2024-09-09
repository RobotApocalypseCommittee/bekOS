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

#include "arch/process_entry.h"

#include "arch/a64/saved_registers.h"
#include "process/process.h"

extern "C" void handle_syscall_a64(InterruptContext& ctx) {
    auto result = handle_syscall(ctx.x[0], ctx.x[1], ctx.x[2], ctx.x[3], ctx.x[4], ctx.x[5], ctx.x[6], ctx.x[7], ctx);
    if (result.has_value()) {
        ctx.set_return_value(result.value());
    } else {
        ctx.set_return_value(-result.error());
    }
}
