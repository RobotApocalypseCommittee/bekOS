#include <stddef.h>
#include <stdint.h>
#include <peripherals/gentimer.h>
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

    unsigned char c;

    while(true) {
        c = uart_getc();
        bad_udelay(2*1000*1000); // Delay 2 seconds
        uart_putc(c);
    }



}