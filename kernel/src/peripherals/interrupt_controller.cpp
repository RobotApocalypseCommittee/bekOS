//
// Created by Joseph on 24/02/2020.
//

#include "peripherals/interrupt_controller.h"
#include "peripherals/peripherals.h"

#define INTCTL_BASE (PERIPHERAL_BASE + 0xB000)
#define INTCTL_BASIC_PENDING (INTCTL_BASE + 0x200)
#define INTCTL_PENDING_1 (INTCTL_BASE + 0x204)
#define INTCTL_PENDING_2 (INTCTL_BASE + 0x208)
#define INTCTL_FIQCTL (INTCTL_BASE + 0x20C)
#define INTCTL_ENIRQ_1 (INTCTL_BASE + 0x210)
#define INTCTL_ENIRQ_2 (INTCTL_BASE + 0x214)
#define INTCTL_EN_BASIC (INTCTL_BASE + 0x218)
#define INTCTL_DISIRQ_1 (INTCTL_BASE + 0x21C)
#define INTCTL_DISIRQ_2 (INTCTL_BASE + 0x220)
#define INTCTL_DIS_BASIC (INTCTL_BASE + 0x224)


void interrupt_controller::enable(interrupt_controller::interrupt_type t) {
    unsigned int x = t;
    if (x >= 64) {
        // 'BASIC' interrupt
        mmio_write(INTCTL_EN_BASIC, 1u << (x-64));
    } else if (x < 32) {
        mmio_write(INTCTL_ENIRQ_1, 1u << x);
    } else {
        mmio_write(INTCTL_ENIRQ_2, 1u << (x-32));
    }
}

void interrupt_controller::disable(interrupt_controller::interrupt_type t) {
    unsigned int x = t;
    if (x >= 64) {
        // 'BASIC' interrupt
        mmio_write(INTCTL_DIS_BASIC, 1u << (x-64));
    } else if (x < 32) {
        mmio_write(INTCTL_DISIRQ_1, 1u << x);
    } else {
        mmio_write(INTCTL_DISIRQ_2, 1u << (x-32));
    }
}
