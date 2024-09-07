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

#include "bek/assertions.h"
#include "bek/types.h"
#include "interrupts/deferred_calls.h"
#include "interrupts/int_ctrl.h"
#include "peripherals/interrupt_controller.h"

extern InterruptController* global_intc;

extern "C" void handle_hardware_interrupt(u64 esr, u64 elr) {
    VERIFY(global_intc);
    global_intc->handle_interrupt();
    enable_interrupts();
    deferred::execute_queue();
    disable_interrupts();
}
