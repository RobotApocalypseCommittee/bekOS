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

#include <usb/hid.h>

#include "api/protocols/kb.h"
#include "library/debug.h"

using DBG = DebugScope<"HID", DebugLevel::WARN>;
using namespace usb;

bek::shared_ptr<::Device> BootHidDevice::probe(const Interface& interface, usb::Device& dev) {
    if (interface.interface_class != 0x3 || interface.interface_subclass != 0x1) {
        return nullptr;
    }

    if (interface.interface_protocol != 0x1 && interface.interface_protocol != 0x2) {
        DBG::errln("HID Boot Protocol Device has unrecognised protocol {}."_sv, interface.interface_protocol);
        return nullptr;
    }

    // We know this is a *boot interface* device!
    if (interface.endpoints.size() != 1) return nullptr;
    auto& ep = interface.endpoints[0];
    if (ep.ttype != TransferType::Interrupt || ep.direction != Direction::In) return nullptr;

    bek::shared_ptr<::Device> dev_ptr = nullptr;
    BootHidDevice* hid_ptr = nullptr;

    if (interface.interface_protocol == 0x01) {
        dev_ptr = bek::adopt_shared(new HidKeyboard(dev, ep.number));
        hid_ptr = static_cast<HidKeyboard*>(dev_ptr.get());
    } else {
        dev_ptr = bek::adopt_shared(new HidMouse(dev, ep.number));
        hid_ptr = static_cast<HidMouse*>(dev_ptr.get());
    }

    dev.schedule_transfer(TransferRequest{
        TransferType::Control,
        Direction::Out,
        0,
        [hid_ptr](bek::optional<mem::own_dma_buffer>, TransferRequest::Result r) {
            hid_ptr->on_set_protocol(r == TransferRequest::Result::Success);
        },
        {},
        SetupPacket{
            SetupPacket::make_req_type(Direction::Out, ControlTransferType::Class, ControlTransferTarget::Interface),
            0x0B, 0, interface.interface_number, 0}});
    return dev_ptr;
}

void BootHidDevice::on_set_protocol(bool success) {
    if (!success) {
        DBG::dbgln("Fatal Error: Failed to switch to boot protocol."_sv);
        return;
    }
    DBG::dbgln("Scheduling first interrupt."_sv);

    auto cb = TransferRequest::Callback{[this](bek::optional<mem::own_dma_buffer> buf, TransferRequest::Result r) {
        on_interrupt(bek::move(*buf), r == TransferRequest::Result::Success);
    }};

    asm volatile("nop");

    auto request = TransferRequest{TransferType::Interrupt,
                        Direction::In,
                        m_interrupt_ep_n,
                        bek::move(cb),
                        m_device.allocate_buffer(m_report_size),
                        {}};

    m_device.schedule_transfer(bek::move(request));
}
void BootHidDevice::on_interrupt(mem::own_dma_buffer buf, bool success) {
    VERIFY(buf.size() == m_report_size);
    if (!success) {
        DBG::errln("Interrupt failed. Will not schedule another."_sv);
        return;
    }

    on_report(buf);

    m_device.schedule_transfer(
        TransferRequest{TransferType::Interrupt,
                        Direction::In,
                        m_interrupt_ep_n,
                        [this](bek::optional<mem::own_dma_buffer> buf, TransferRequest::Result r) {
                            on_interrupt(bek::move(*buf), r == TransferRequest::Result::Success);
                        },
                        bek::move(buf),
                        {}});
}

protocols::kb::Report HidKeyboard::get_report() const {
    protocols::kb::Report protocol_report = {m_report.modifier_keys, {}};
    bek::memcopy(protocol_report.keys, m_report.keys, 6);
    return protocol_report;
}
void HidKeyboard::on_report(mem::own_dma_buffer& buf) {
    auto new_report = buf.view().get_at<Report>(0);
    if (m_report != new_report) {
        DBG::dbgln("Report: {}"_sv, new_report);
        m_report = new_report;
    }
}

constexpr inline const char* modifier_key_strings[] = {"LCtrl", "LShift", "LAlt", "LWin",
                                                       "RCtrl", "RShift", "RAlt", "RWin"};
constexpr char try_convert_keycode(u8 c) {
    if (c >= 4 && c <= 29) {
        return c + 61;
    } else if (30 <= c && c <= 38) {
        return c + 19;
    } else if (c == 39) {
        return '0';
    } else {
        return 0;
    }
}

void HidMouse::on_report(mem::own_dma_buffer& buf) {
    auto new_report = buf.view().get_at<Report>(0);
    DBG::dbgln("Report: {}, ({}, {})"_sv, new_report.buttons, new_report.x, new_report.y);
    update_report(new_report.buttons, new_report.x, new_report.y);
}
void usb::bek_basic_format(bek::OutputStream& out, const HidKeyboard::Report& report) {
    bool first = true;
    if (report.modifier_keys != 0) {
        for (int i = 0; i < 8; i++) {
            if (report.modifier_keys & (1u << i)) {
                if (first) {
                    first = false;
                } else {
                    out.write('+');
                }
                out.write(bek::str_view(modifier_key_strings[i]));
            }
        }
    }
    for (auto key : report.keys) {
        if (key >= 4) {
            if (first) {
                first = false;
            } else {
                out.write('+');
            }
            if (try_convert_keycode(key)) {
                out.write(try_convert_keycode(key));
            } else {
                bek::format_to(out, "{}"_sv, key);
            }
        }
    }
}
