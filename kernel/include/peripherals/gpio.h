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

#ifndef BEKOS_GPIO_H
#define BEKOS_GPIO_H

#include "peripherals/peripherals.h"



// The GPIO registers base address.
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000) // for raspi2 & 3, 0x20200000 for raspi1



// The offsets for each register.

// Controls selection of alts
#define GPFSEL0 (0x00)
#define GPFSEL1         (0x04)
#define GPFSEL2 (0x08)
#define GPFSEL3 (0x0C)
#define GPFSEL4 (0x10)
#define GPFSEL5 (0x14)

#define GPSET0          (0x1C)
#define GPSET1          (0x20)
#define GPCLR0          (0x28)
#define GPCLR1          (0x2C)

// Controls actuation of pull up/down to ALL GPIO pins.
#define GPPUD (0x94)

// Controls actuation of pull up/down for specific GPIO pin.
#define GPPUDCLK0 (0x98)
#define GPPUDCLK1 (0x9C)

enum PullupState {
    Disabled = 0,
    PullDown = 1,
    PullUp = 2
};

enum PinFunction {
    Input = 0b000,
    Output = 0b001,
    Alt0 = 0b100,
    Alt1 = 0b101,
    Alt2 = 0b110,
    Alt3 = 0b111,
    Alt4 = 0b011,
    Alt5 = 0b010
};

static inline void delay(i32 count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
    : "=r"(count): [count]"0"(count) : "cc");
}

class GPIO: mmio_register_device {
public:
    explicit GPIO(uPtr gpio_base): mmio_register_device(gpio_base) {}
    void set_pullups(PullupState state, u64 pin_map) {
        write_register(GPPUD, state);
        delay(150);

        // Bottom half
        if (pin_map & 0xFFFFFFFF) {
            write_register(GPPUDCLK0, pin_map & 0xFFFFFFFF);
        }
        // Top half
        if (pin_map >> 32) {
            write_register(GPPUDCLK1, pin_map >> 32);
        }
        delay(150);
        write_register(GPPUDCLK0, 0);
        write_register(GPPUDCLK1, 0);
    }
    void set_pin_function(PinFunction function, unsigned int pin) {
        auto reg = GPFSEL0 + (pin / 10) * 4;
        unsigned bit_offset = (pin % 10) * 3;
        auto val = read_register(reg);
        val &= ~(0b111u << bit_offset);
        val |= (static_cast<u32>(function) << bit_offset);
        write_register(reg, val);
    }
    void set_pin(bool high, unsigned int pin) {
        uPtr reg = high ? (pin < 32 ? GPSET0 : GPSET1) : (pin < 32 ? GPCLR0 : GPCLR1);
        if (pin >= 32) { pin -= 32; }
        write_register(reg, 1 << pin);
    }
};



#endif //BEKOS_GPIO_H
