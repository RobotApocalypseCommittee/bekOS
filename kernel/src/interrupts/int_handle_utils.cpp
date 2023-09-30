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
#include <peripherals/gpio.h>
#include <peripherals/interrupt_controller.h>
#include <peripherals/uart.h>

#include "library/debug.h"

extern InterruptController* global_intc;
extern PL011* serial;

using DBG = DebugScope<"ERR", true>;

constexpr bek::str_view str_int_type(unsigned long type) {
    switch (type % 4) {
        case 0:
            return "Synchronous"_sv;
        case 1:
            return "IRQ"_sv;
        case 2:
            return "FIQ"_sv;
        case 3:
            return "SError"_sv;
        default:
            ASSERT(false);
    }
}

constexpr bek::str_view str_int_level(unsigned long type) {
    switch (type / 4) {
        case 0:
            return "EL1, SP0"_sv;
        case 1:
            return "EL1, SP1"_sv;
        case 2:
            return "EL0, 64bit"_sv;
        case 3:
            return "EL1, 32bit"_sv;
        default:
            return "?"_sv;
    }
}

constexpr bek::str_view str_exception_type(unsigned long esr) {
    switch (esr >> 26) {
        case 0b000000:
            return "Unknown"_sv;
        case 0b000001:
            return "Trapped WFI/WFE"_sv;
        case 0b001110:
            return "Illegal execution"_sv;
        case 0b010101:
            return "System call"_sv;
        case 0b100000:
            return "Instruction abort, lower EL"_sv;
        case 0b100001:
            return "Instruction abort, same EL"_sv;
        case 0b100010:
            return "Instruction alignment fault"_sv;
        case 0b100100:
            return "Data abort, lower EL"_sv;
        case 0b100101:
            return "Data abort, same EL"_sv;
        case 0b100110:
            return "Stack alignment fault"_sv;
        case 0b101100:
            return "Floating point"_sv;
        default:
            return "Unknown"_sv;
    }
}

constexpr bek::str_view str_data_abort_fault(unsigned long esr) {
    switch ((esr >> 2) & 0x3) {
        case 0:
            return "Address size fault"_sv;
        case 1:
            return "Translation fault"_sv;
        case 2:
            return "Access flag fault"_sv;
        case 3:
            return "Permission fault"_sv;
        default:
            ASSERT(false);
    }
}

constexpr u8 data_abort_level(unsigned long esr) { return esr & 0x3; }

/**
 * common exception handler
 */
extern "C"
void unknown_int_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // print out interruption type

    DBG::dbgln("{} exception at {}: {}"_sv, str_int_type(type), str_int_level(type),
               str_exception_type(esr));
    // decode data abort cause
    if(esr>>26==0b100100 || esr>>26==0b100101) {
        DBG::dbgln("   {} at level {}"_sv, str_data_abort_fault(esr), data_abort_level(esr));
    }

    // dump registers
    DBG::dbgln("   ESR_EL1  {:XL} ELR_EL1 {:XL}"_sv, esr, elr);
    DBG::dbgln("   SPSR_EL1 {:XL} FAR_EL1 {:XL}"_sv, spsr, far);

    // no return from exception for now
    while(1);
}

extern "C"
void handle_interrupt(unsigned long esr, unsigned long elr) {
    VERIFY(global_intc);
    global_intc->handle_interrupt();
}