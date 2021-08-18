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
#ifndef BEKOS_UART_H
#define BEKOS_UART_H

#include <library/types.h>

#include "peripherals/peripherals.h"
#include "gpio.h"

// The base address for UART.
#define UART0_BASE  (PERIPHERAL_BASE + 0x201000) // for raspi2 & 3, 0x20201000 for raspi1

// The offsets for reach register for the UART.
#define UART0_DR      (0x00)
#define UART0_RSRECR  (0x04)
#define UART0_FR      (0x18)
#define UART0_ILPR    (0x20)
#define UART0_IBRD    (0x24)
#define UART0_FBRD    (0x28)
#define UART0_LCRH    (0x2C)
#define UART0_CR      (0x30)
#define UART0_IFLS    (0x34)
#define UART0_IMSC    (0x38)
#define UART0_RIS     (0x3C)
#define UART0_MIS     (0x40)
#define UART0_ICR     (0x44)
#define UART0_DMACR   (0x48)
#define UART0_ITCR    (0x80)
#define UART0_ITIP    (0x84)
#define UART0_ITOP    (0x88)
#define UART0_TDR     (0x8C)

class PL011: mmio_register_device {
    explicit PL011(uPtr uart_base, GPIO& gpio);
    void putc(unsigned char c);
    unsigned char getc();
    void puts(const char* str);
};
#define AUX_BASE (PERIPHERAL_BASE + 0x215000)
#define AUX_ENABLES     (0x04)
#define AUX_MU_IO_REG   (0x40)
#define AUX_MU_IER_REG  (0x44)
#define AUX_MU_IIR_REG  (0x48)
#define AUX_MU_LCR_REG  (0x4C)
#define AUX_MU_MCR_REG  (0x50)
#define AUX_MU_LSR_REG  (0x54)
#define AUX_MU_MSR_REG  (0x58)
#define AUX_MU_SCRATCH  (0x5C)
#define AUX_MU_CNTL_REG (0x60)
#define AUX_MU_STAT_REG (0x64)
#define AUX_MU_BAUD_REG (0x68)

class mini_uart: mmio_register_device {
public:
    explicit mini_uart(uPtr uart_base, GPIO& gpio);
    void putc(unsigned char c);
    unsigned char getc();
    void puts(const char* str);
    void puthex(unsigned long x);
};


#endif //BEKOS_UART_H
