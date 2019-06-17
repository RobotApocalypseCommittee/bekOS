//
// Created by Joseph on 29/03/2019.
//

#ifndef BEKOS_UART_H
#define BEKOS_UART_H

#include "peripherals/peripherals.h"

// The base address for UART.
#define UART0_BASE  (PERIPHERAL_BASE + 0x201000) // for raspi2 & 3, 0x20201000 for raspi1

// The offsets for reach register for the UART.
#define UART0_DR      (UART0_BASE + 0x00)
#define UART0_RSRECR  (UART0_BASE + 0x04)
#define UART0_FR      (UART0_BASE + 0x18)
#define UART0_ILPR    (UART0_BASE + 0x20)
#define UART0_IBRD    (UART0_BASE + 0x24)
#define UART0_FBRD    (UART0_BASE + 0x28)
#define UART0_LCRH    (UART0_BASE + 0x2C)
#define UART0_CR      (UART0_BASE + 0x30)
#define UART0_IFLS    (UART0_BASE + 0x34)
#define UART0_IMSC    (UART0_BASE + 0x38)
#define UART0_RIS     (UART0_BASE + 0x3C)
#define UART0_MIS     (UART0_BASE + 0x40)
#define UART0_ICR     (UART0_BASE + 0x44)
#define UART0_DMACR   (UART0_BASE + 0x48)
#define UART0_ITCR    (UART0_BASE + 0x80)
#define UART0_ITIP    (UART0_BASE + 0x84)
#define UART0_ITOP    (UART0_BASE + 0x88)
#define UART0_TDR     (UART0_BASE + 0x8C)

void uart_init();
void uart_putc(unsigned char c);
unsigned char uart_getc();
void uart_puts(const char* str);

#endif //BEKOS_UART_H
