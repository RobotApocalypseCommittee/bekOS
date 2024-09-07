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

#include <usb/hid.h>

#include "api/protocols/kb.h"
#include "library/debug.h"

using DBG = DebugScope<"HID", true>;
using namespace usb;

bek::shared_ptr<HidKeyboard> HidKeyboard::probe(const Interface& interface, usb::Device& dev) {
    if (interface.interface_class != 0x3 || interface.interface_subclass != 0x1 ||
        interface.interface_protocol != 0x1) {
        return nullptr;
    }

    // We know this is a *boot interface* keyboard!
    if (interface.endpoints.size() != 1) return nullptr;
    auto& ep = interface.endpoints[0];
    if (ep.ttype != TransferType::Interrupt || ep.direction != Direction::In) return nullptr;

    auto ptr  = new HidKeyboard(dev, ep.number);
    auto& hid = *ptr;

    dev.schedule_transfer(TransferRequest{
        TransferType::Control,
        Direction::Out,
        0,
        [&hid](bek::optional<mem::own_dma_buffer>, TransferRequest::Result r) {
            hid.on_set_protocol(r == TransferRequest::Result::Success);
        },
        {},
        SetupPacket{
            SetupPacket::make_req_type(Direction::Out, ControlTransferType::Class, ControlTransferTarget::Interface),
            0x0B, 0, interface.interface_number, 0}});
    return bek::adopt_shared(ptr);
}
void HidKeyboard::on_set_protocol(bool success) {
    if (!success) {
        DBG::dbgln("Fatal Error: Failed to switch to boot protocol."_sv);
        return;
    }
    DBG::dbgln("Scheduling first interrupt."_sv);

    m_device.schedule_transfer(
        TransferRequest{TransferType::Interrupt,
                        Direction::In,
                        m_interrupt_ep_n,
                        [this](bek::optional<mem::own_dma_buffer> buf, TransferRequest::Result r) {
                            on_interrupt(bek::move(*buf), r == TransferRequest::Result::Success);
                        },
                        m_device.allocate_buffer(8),
                        {}});
}
void HidKeyboard::on_interrupt(mem::own_dma_buffer buf, bool success) {
    VERIFY(buf.size() == 8);
    if (!success) {
        DBG::dbgln("Interrupt failed. Will not schedule another."_sv);
        return;
    }

    auto new_report = buf.view().get_at<Report>(0);
    if (m_report != new_report) {
        DBG::dbgln("Report: {}"_sv, new_report);
        m_report = new_report;
    }
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
