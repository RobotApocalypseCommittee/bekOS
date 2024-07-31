/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef BEKOS_UART_H
#define BEKOS_UART_H

#include "bek/types.h"
#include "device.h"
#include "device_tree.h"
#include "gpio.h"
#include "library/format_core.h"
#include "peripherals/peripherals.h"

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

class UART : public bek::OutputStream, public Device {
public:
    Kind kind() const override;
    void reserve(uSize n) override;
    void puthex(unsigned long x);

    virtual char getc() = 0;

private:
    using DeviceType                         = UART;
    static constexpr Device::Kind DeviceKind = Device::Kind::UART;
};

class PL011 : mmio_register_device, public UART {
public:
    explicit PL011(uPtr uart_base, u32 base_clock, u32 baudrate = 115200, u32 data_bits = 8,
                   u32 stop_bits = 1);
    char getc() override;
    void write(bek::str_view str) override;
    void write(char c) override;

    static dev_tree::DevStatus probe_devtree(dev_tree::Node& node, dev_tree::device_tree& tree);

private:
    u32 m_base_clock;
    u32 m_baudrate;
    u32 m_data_bits;
    u32 m_stop_bits;
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

class mini_uart : mmio_register_device, public UART {
public:
    explicit mini_uart(uPtr uart_base, GPIO& gpio);
    char getc() override;
    void write(bek::str_view str) override;
    void write(char c) override;
};


#endif //BEKOS_UART_H
