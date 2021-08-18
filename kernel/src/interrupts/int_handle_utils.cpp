/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <interrupts/int_ctrl.h>
#include <memory_locations.h>
#include <peripherals/gpio.h>
#include <peripherals/interrupt_controller.h>
#include <peripherals/uart.h>

#include "printf.h"

extern interrupt_controller interruptController;

/**
 * common exception handler
 */
extern "C"
void unknown_int_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    GPIO gpio(mapped_address(GPIO_BASE));
    mini_uart uart(mapped_address(AUX_BASE), gpio);
    // print out interruption type
    switch(type % 4) {
        case 0: uart.puts("Synchronous"); break;
        case 1: uart.puts("IRQ"); break;
        case 2: uart.puts("FIQ"); break;
        case 3: uart.puts("SError"); break;
    }
    uart.puts(", ");
    switch(type / 4) {
        case 0: uart.puts("EL1, SP0"); break;
        case 1: uart.puts("EL1, SP1"); break;
        case 2: uart.puts("EL0, 64bit"); break;
        case 3: uart.puts("EL1, 32bit"); break;
    }
    uart.puts(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr>>26) {
        case 0b000000: uart.puts("Unknown"); break;
        case 0b000001: uart.puts("Trapped WFI/WFE"); break;
        case 0b001110: uart.puts("Illegal execution"); break;
        case 0b010101: uart.puts("System call"); break;
        case 0b100000: uart.puts("Instruction abort, lower EL"); break;
        case 0b100001: uart.puts("Instruction abort, same EL"); break;
        case 0b100010: uart.puts("Instruction alignment fault"); break;
        case 0b100100: uart.puts("Data abort, lower EL"); break;
        case 0b100101: uart.puts("Data abort, same EL"); break;
        case 0b100110: uart.puts("Stack alignment fault"); break;
        case 0b101100: uart.puts("Floating point"); break;
        default: uart.puts("Unknown"); break;
    }
    // decode data abort cause
    if(esr>>26==0b100100 || esr>>26==0b100101) {
        uart.puts(", ");
        switch((esr>>2)&0x3) {
            case 0: uart.puts("Address size fault"); break;
            case 1: uart.puts("Translation fault"); break;
            case 2: uart.puts("Access flag fault"); break;
            case 3: uart.puts("Permission fault"); break;
        }
        switch(esr&0x3) {
            case 0: uart.puts(" at level 0"); break;
            case 1: uart.puts(" at level 1"); break;
            case 2: uart.puts(" at level 2"); break;
            case 3: uart.puts(" at level 3"); break;
        }
    }
    // dump registers
    uart.puts(":\n  ESR_EL1 ");
    uart.puthex(esr>>32);
    uart.puthex(esr);
    uart.puts(" ELR_EL1 ");
    uart.puthex(elr>>32);
    uart.puthex(elr);
    uart.puts("\n SPSR_EL1 ");
    uart.puthex(spsr>>32);
    uart.puthex(spsr);
    uart.puts(" FAR_EL1 ");
    uart.puthex(far>>32);
    uart.puthex(far);
    uart.puts("\n");
    // no return from exception for now
    while(1);
}
/*
extern "C"
void handle_interrupt(unsigned long esr, unsigned long elr) {
    //printf("Interrupt: ESR = %X, ELR = %X\n", esr, elr);
    // Try BCM
    if (interruptController.handle()) {
        // Success
        return;
    } else {
        // TODO: Disaster
        printf("Disaster");
        return;
    }
}
 */