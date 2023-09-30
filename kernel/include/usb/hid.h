/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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
#include "usb.h"
namespace usb {

class HidKeyboard : public Functionality {
public:
    struct Report {
        u8 modifier_keys;
        u8 padding;
        u8 keys[6];
        bool operator==(const Report&) const = default;
    };

    static bek::own_ptr<Functionality> probe_mouse(const Interface& interface, Device&);

private:
    void on_set_protocol(bool success);
    void on_interrupt(mem::own_dma_buffer buf, bool success);

    HidKeyboard(Device& device, u8 interrupt_ep_n)
        : m_device(device), m_interrupt_ep_n(interrupt_ep_n) {}
    Device& m_device;
    u8 m_interrupt_ep_n;
    Report m_report{};
};

void bek_basic_format(bek::OutputStream&, const HidKeyboard::Report&);

}  // namespace usb

#endif  // BEKOS_HID_H
