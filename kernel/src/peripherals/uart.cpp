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


#include "peripherals/uart.h"

PL011::PL011(uPtr uart_base, GPIO& gpio): mmio_register_device(uart_base) {
    // Disable UART0.
    write_register(UART0_CR, 0x00000000);

    // Setup the GPIO pin 14 && 15.
    gpio.set_pullups(Disabled, (1 << 14) | (1 << 15));

    // Clear pending interrupts.
    write_register(UART0_ICR, 0x7FF);

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // Fraction part register = (Fractional part * 64) + 0.5
    // UART_CLOCK = 3000000; Baud = 115200.

    // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
    write_register(UART0_IBRD, 1);

    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    write_register(UART0_FBRD, 40);

    // Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
    write_register(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Mask all interrupts.
    write_register(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    // Enable UART0, receive & transfer part of UART.
    write_register(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}
void PL011::putc(unsigned char c) {
    // Wait for UART to be ready
    while (read_register(UART0_FR) & (1 << 5)) {}
    write_register(UART0_DR, c);
}
unsigned char PL011::getc() {
    // Wait to receive
    while (read_register(UART0_FR) & (1 << 4)) {}
    return static_cast<unsigned char>(read_register(UART0_DR));
}
void PL011::puts(const char* str) {
    for (uSize i = 0; str[i] != '\0'; i++) {
        putc(str[i]);
    }
}

mini_uart::mini_uart(uPtr uart_base, GPIO& gpio): mmio_register_device(uart_base) {

    write_register(AUX_ENABLES, read_register(AUX_ENABLES) | 1);
    write_register(AUX_MU_CNTL_REG, 0);
    write_register(AUX_MU_LCR_REG, 3);
    write_register(AUX_MU_MCR_REG, 0);
    write_register(AUX_MU_IER_REG, 0);
    write_register(AUX_MU_IIR_REG, 0xC6);
    write_register(AUX_MU_BAUD_REG, 270);

    gpio.set_pin_function(Alt5, 14);
    gpio.set_pin_function(Alt5, 15);

    gpio.set_pullups(Disabled, (1 << 14) | (1 << 15));

    write_register(AUX_MU_CNTL_REG, 3);
}
void mini_uart::putc(unsigned char c) {
    while (!(read_register(AUX_MU_LSR_REG) & 0x20)) {}
    write_register(AUX_MU_IO_REG, c);
}
unsigned char mini_uart::getc() {
    while (!(read_register(AUX_MU_LSR_REG) & 0x01)) {}
    return read_register(AUX_MU_IO_REG) & 0xFF;
}

void mini_uart::puts(const char* str) {
    for (uSize i = 0; str[i] != '\0'; i++) {
        putc(str[i]);
    }
}
void mini_uart::puthex(unsigned long x) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(x>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        putc(n);
    }
}
