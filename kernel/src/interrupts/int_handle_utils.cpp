//
// Created by Joseph on 29/01/2020.
//
#include <memory_locations.h>
#include <peripherals/gpio.h>
#include <peripherals/interrupt_controller.h>
#include "printf.h"

extern interrupt_controller interruptController;

extern "C"
void show_unknown_int_complaint(unsigned long esr, unsigned long elr) {
    unsigned x = mmio_read(PERIPHERAL_BASE + 0xB000 + 0x204);
    printf("Unknown Interrupt: ESR = %X, ELR = %X, BCM = %X\n", esr, elr, x);
}

extern "C"
void handle_interrupt(unsigned long esr, unsigned long elr) {
    unsigned x = mmio_read(PERIPHERAL_BASE + 0xB000 + 0x204);
    printf("Interrupt: ESR = %X, ELR = %X, BCM = %X\n", esr, elr, x);
    if (esr == 0) {
        // Try BCM
        if (interruptController.handle()) {
            // Success
            return;
        } else {
            // TODO: Disaster
            printf("Disaster");
            return;
        }
    } else {
        // Other interrupt
        return;
    }
}