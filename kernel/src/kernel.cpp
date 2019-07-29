/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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

#include <stddef.h>
#include <stdint.h>
#include <peripherals/gentimer.h>
#include <peripherals/property_tags.h>
#include "peripherals/uart.h"
#include "printf.h"


// Needed for printf
void _putchar(char character) {
    uart_putc(character);
}

extern "C" /* Use C linkage for kernel_main. */
void kernel_main(uint32_t el, uint32_t r1, uint32_t atags)
{
    // Declare as unused
    (void) r1;
    (void) atags;

    uart_init();
    uart_puts("Hello, kernel World!\r\n");

    printf("The current exception level is %u\n", el);
    printf("General Timer Frequency: %u\n", getClockFrequency());

    PropertyTagClockRate clockTag;
    clockTag.clock_id = 0x000000001;
    property_tags tags;
    bool x = tags.request_tag(0x00030002, &clockTag, sizeof(PropertyTagClockRate));
    if (x) {
        printf("EMMC Clock Rate: %u\n", clockTag.rate);
    }

    clockTag.clock_id = 0x000000006;
    x = tags.request_tag(0x00030002, &clockTag, sizeof(PropertyTagClockRate));
    if (x) {
        printf("H264 Clock Rate: %u\n", clockTag.rate);
    }

    unsigned char c;

    while(true) {
        c = uart_getc();
        bad_udelay(2*1000*1000); // Delay 2 seconds
        uart_putc(c);
    }



}