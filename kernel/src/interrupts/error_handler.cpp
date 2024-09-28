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
#include <peripherals/gpio.h>
#include <peripherals/interrupt_controller.h>
#include <peripherals/uart.h>

#include "library/debug.h"

using DBG = DebugScope<"ERR", DebugLevel::DEBUG>;

struct ExceptionSyndrome {
    struct AbortInformation {
        constexpr AbortInformation(u32 iss, [[maybe_unused]] u32 iss2)
            : fault_status_code{static_cast<u8>(iss & 0b111111)}, wnr{static_cast<bool>(iss & (0b1 << 6))} {}
        constexpr bek::str_view fault_kind() const {
            switch ((fault_status_code >> 2) & 0b1111) {
                case 0:
                    return "Address size fault"_sv;
                case 1:
                    return "Translation fault"_sv;
                case 2:
                    return "Access flag fault"_sv;
                case 3:
                    return "Permission fault"_sv;
                case 4:
                    return "Synchronous External abort: unclassified"_sv;
                case 5:
                    return "Synchronous External abort: table walk/update"_sv;
                case 6:
                    return "Synchronous parity or ECC error: unclassified"_sv;
                case 7:
                    return "Synchronous parity or ECC error: table walk/update"_sv;
                case 8:
                    return "Alignment Fault"_sv;
                default:
                    return "Unknown: Undecoded"_sv;
            }
        }
        constexpr bool was_write() const { return wnr; }
        constexpr u8 table_level() const { return (fault_status_code & 0b11); }
        u8 fault_status_code;
        bool wnr;
    };
    constexpr explicit ExceptionSyndrome(u64 esr)
        : exception_class{static_cast<u8>((esr >> 26) & 0b111111)},
          iss{static_cast<u32>(esr & 0x1FFFFF)},
          iss2{static_cast<u32>(esr >> 32)} {}
    constexpr bek::str_view exception_class_description() const {
        switch (exception_class) {
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
                return "Unknown (Decoding Not Provided)"_sv;
        }
    }

    constexpr bool is_data_abort() const { return (exception_class == 0b100100) || (exception_class == 0b100101); }
    constexpr bool is_instruction_abort() const {
        return (exception_class == 0b100000) || (exception_class == 0b100001);
    }
    constexpr AbortInformation abort_information() const { return AbortInformation{iss, iss2}; }

    u8 exception_class;
    u32 iss;
    u32 iss2;
};

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
            ASSERT_UNREACHABLE();
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

/**
 * common exception handler
 */
extern "C" void unknown_int_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr,
                                    unsigned long far) {
    // print out interruption type
    ExceptionSyndrome syndrome{esr};
    DBG::errln("{} exception at {}: {}"_sv, str_int_type(type), str_int_level(type),
               syndrome.exception_class_description());
    // decode data abort cause
    if (syndrome.is_data_abort() || syndrome.is_instruction_abort()) {
        auto abort_info = syndrome.abort_information();
        DBG::errln("   {} (at level {}) ({})"_sv, abort_info.fault_kind(), abort_info.table_level(),
                   abort_info.was_write() ? "write"_sv : "read"_sv);
    }

    // dump registers
    DBG::errln("   ESR_EL1  {:XL} ELR_EL1 {:XL}"_sv, esr, elr);
    DBG::errln("   SPSR_EL1 {:XL} FAR_EL1 {:XL}"_sv, spsr, far);

    // no return from exception for now
    while (1)
        ;
}
