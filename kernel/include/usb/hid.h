/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024-2025 Bekos Contributors
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

#ifndef BEKOS_HID_H
#define BEKOS_HID_H

#include "descriptors.h"
#include "peripherals/keyboard.h"
#include "peripherals/mouse.h"
#include "usb.h"

namespace usb {

class BootHidDevice: public Functionality {
public:
    static bek::shared_ptr<::Device> probe(const Interface& interface, usb::Device& dev);

protected:
 BootHidDevice(usb::Device& device, u8 interrupt_ep_n, u8 report_size)
    : m_device(device), m_interrupt_ep_n(interrupt_ep_n), m_report_size(report_size) {}
    void on_set_protocol(bool success);
    void on_interrupt(mem::own_dma_buffer buf, bool success);
    virtual void on_report(mem::own_dma_buffer& buf) = 0;

    usb::Device& m_device;
    u8 m_interrupt_ep_n;
    u8 m_report_size;
};


class HidKeyboard : public BootHidDevice, public KeyboardDevice {
public:
    struct Report {
        u8 modifier_keys;
        u8 padding;
        u8 keys[6];
        bool operator==(const Report&) const = default;
    };
    static_assert(sizeof(Report) == 8);

    [[nodiscard]] protocols::kb::Report get_report() const override;

    HidKeyboard(usb::Device& device, u8 interrupt_ep_n) : BootHidDevice(device, interrupt_ep_n, 8) {}
protected:
    void on_report(mem::own_dma_buffer& buf) override;
private:
    Report m_report{};
};

class HidMouse: public BootHidDevice, public MouseDevice {
public:
    struct Report {
        u8 buttons;
        i8 x;
        i8 y;
        bool operator==(const Report&) const = default;
    };
    static_assert(sizeof(Report) == 3);

    HidMouse(usb::Device& device, u8 interrupt_ep_n): BootHidDevice(device, interrupt_ep_n, 3) {}
protected:
    void on_report(mem::own_dma_buffer& buf) override;
private:
    protocols::mouse::Report m_report{};
};




void bek_basic_format(bek::OutputStream&, const HidKeyboard::Report&);

}  // namespace usb

#endif  // BEKOS_HID_H
