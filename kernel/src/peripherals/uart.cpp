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

#include "peripherals/uart.h"

#include "library/debug.h"
#include "peripherals/clock.h"

using DBG = DebugScope<"UART", true>;

extern bek::OutputStream* debug_stream;

char PL011::getc() {
    // Wait to receive
    while (read_register(UART0_FR) & (1 << 4)) {
    }

    return static_cast<unsigned char>(read_register(UART0_DR));
}

PL011::PL011(uPtr uart_base, u32 base_clock, u32 baudrate, u32 data_bits, u32 stop_bits)
    : mmio_register_device(uart_base),
      m_base_clock(base_clock),
      m_baudrate(baudrate),
      m_data_bits(data_bits),
      m_stop_bits(stop_bits) {
    // Disable UART0.
    write_register(UART0_CR, 0x00000000);

    // Clear pending interrupts.
    write_register(UART0_ICR, 0x7FF);

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // Fraction part register = (Fractional part * 64) + 0.5
    // So multiply all by 128, then shift appropriately. Divider = 8*clock/baud
    // UART_CLOCK = 3000000; Baud = 115200.

    u32 divider     = 8 * base_clock / baudrate;
    u32 int_divider = divider / 128;
    // Only 6 bits.
    u32 frac_divider = ((divider + 1) / 2) & 0b111111;

    write_register(UART0_IBRD, int_divider);

    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    write_register(UART0_FBRD, frac_divider);

    // TODO: Stop bits and word length.

    // Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
    write_register(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Mask all interrupts.
    write_register(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) |
                                   (1 << 9) | (1 << 10));

    // Enable UART0, receive & transfer part of UART.
    write_register(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void PL011::write(bek::str_view str) {
    for (char c : str) {
        write(c);
    }
}
void PL011::write(char c) {
    // Wait for UART to be ready
    while (read_register(UART0_FR) & (1 << 5)) {
    }
    write_register(UART0_DR, c);
}

dev_tree::DevStatus PL011::probe_devtree(dev_tree::Node& node, dev_tree::device_tree& tree, dev_tree::probe_ctx& ctx) {
    if (!(node.compatible.size() && node.compatible[0] == "arm,pl011"_sv))
        return dev_tree::DevStatus::Unrecognised;
    // Get first clock.
    auto prop_opt = node.get_property("clocks"_sv);
    if (!prop_opt) return dev_tree::DevStatus::Failure;
    auto phandle = prop_opt->get_at_be<u32>(0);
    DBG::dbgln("Seeking phandle {}."_sv, phandle);
    auto clk_node_it = tree.phandles.find(phandle);
    if (!clk_node_it) return dev_tree::DevStatus::Failure;
    auto& clk_node = **clk_node_it;
    if (clk_node.nodeStatus == dev_tree::DevStatus::Waiting ||
        clk_node.nodeStatus == dev_tree::DevStatus::Unprobed) {
        return dev_tree::DevStatus::Waiting;
    } else if (clk_node.nodeStatus == dev_tree::DevStatus::Success) {
        // Downcast
        auto& clk_dev = *Device::as<Clock>(clk_node.attached_device.get());
        auto reg = dev_tree::get_regions_from_reg(node);
        // FIXME: We don't map, because we hackily already do in kernel.
        // TODO: Bus Mapping
        node.attached_device = bek::make_own<PL011>(VA_IDENT_OFFSET + reg[0].start.get(), clk_dev.get_frequency());
        // TODO: Properly do this.
        debug_stream = static_cast<PL011*>(node.attached_device.get());
        DBG::dbgln("Successfully probed {}"_sv, node.name);
        return dev_tree::DevStatus::Success;
    } else {
        return dev_tree::DevStatus::Failure;
    }
}

mini_uart::mini_uart(uPtr uart_base, GPIO& gpio) : mmio_register_device(uart_base) {
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
void mini_uart::write(char c) {
    while (!(read_register(AUX_MU_LSR_REG) & 0x20)) {
    }
    write_register(AUX_MU_IO_REG, c);
}
char mini_uart::getc() {
    while (!(read_register(AUX_MU_LSR_REG) & 0x01)) {
    }
    return read_register(AUX_MU_IO_REG) & 0xFF;
}

void mini_uart::write(bek::str_view str) {
    for (char c : str) {
        write(c);
    }
}

Device::Kind UART::kind() const { return Kind::UART; }
void UART::reserve(uSize) {}
void UART::puthex(unsigned long x) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (x >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        write(n);
    }
}
bek::str_view UART::preferred_name_prefix() const { return "generic.serial"_sv; }
