//
// Created by Joseph on 29/03/2019.
//

#ifndef BEKOS_GPIO_H
#define BEKOS_GPIO_H

#include "peripherals/peripherals.h"

// The GPIO registers base address.
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000) // for raspi2 & 3, 0x20200000 for raspi1

// The offsets for reach register.

// Controls actuation of pull up/down to ALL GPIO pins.
#define GPPUD (GPIO_BASE + 0x94)

// Controls actuation of pull up/down for specific GPIO pin.
#define GPPUDCLK0 (GPIO_BASE + 0x98)

#endif //BEKOS_GPIO_H
