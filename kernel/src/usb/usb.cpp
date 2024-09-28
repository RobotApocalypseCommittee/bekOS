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

#include <usb/usb.h>

#include "library/debug.h"
#include "usb/descriptors.h"
#include "usb/hid.h"

using DBG = DebugScope<"USB", DebugLevel::WARN>;
using namespace usb;

TransferRequest make_descriptor_request(DescriptorBase::DescriptorType type, uSize length,
                                        TransferRequest::Callback&& cb, usb::Device& dev,
                                        u8 index = 0) {
    return {
        TransferType::Control,
        // Doesn't matter
        Direction::In, 0, bek::move(cb), dev.allocate_buffer(length),
        SetupPacket{SetupPacket::make_req_type(usb::Direction::In, ControlTransferType::Standard,
                                               ControlTransferTarget::Device),
                    0x06, static_cast<u16>((type << 8) | index), 0, static_cast<u16>(length)}};
}

void probe_device(usb::Device& device, const Interface& interface) {
    // FIXME: Make general.
    auto ptr = HidKeyboard::probe(interface, device);
    if (ptr) {
        DBG::dbgln("Probe keyboard success."_sv);
        DeviceRegistry::the().register_device("generic.usb.keyboard"_sv, ptr);
    }
}

void begin_enumeration(usb::Device& device) {
    device.schedule_transfer(make_descriptor_request(
        usb::DescriptorBase::Device, sizeof(DeviceDescriptor),
        [&device](bek::optional<mem::own_dma_buffer> buf, TransferRequest::Result res) {
            VERIFY(res == usb::TransferRequest::Result::Success);
            auto& dev_desc = buf->view().get_at<DeviceDescriptor>(0);
            DBG::dbgln("Device Descriptor: Vendor {}, Product {}, with {} configurations."_sv,
                       dev_desc.vendor_id, dev_desc.product_id, dev_desc.configuration_count);

            device.schedule_transfer(make_descriptor_request(
                usb::DescriptorBase::Configuration, sizeof(ConfigurationDescriptor),
                [&device](bek::optional<mem::own_dma_buffer> buf, TransferRequest::Result res) {
                    VERIFY(res == usb::TransferRequest::Result::Success);
                    auto& descriptor = buf->view().get_at<ConfigurationDescriptor>(0);
                    DBG::dbgln("Configuration Descriptor: {} interfaces, with total length {}."_sv,
                               descriptor.num_interfaces, descriptor.total_length);

                    device.schedule_transfer(make_descriptor_request(
                        usb::DescriptorBase::Configuration, descriptor.total_length,
                        [&device](bek::optional<mem::own_dma_buffer> buf,
                                  TransferRequest::Result res) {
                            VERIFY(res == usb::TransferRequest::Result::Success);
                            auto& configuration = buf->view().get_at<ConfigurationDescriptor>(0);
                            auto interfaces     = usb::parse_configuration({buf->raw_view()});
                            bek::vector<Endpoint> configuration_endpoints;
                            for (auto& interface : interfaces) {
                                DBG::dbgln(
                                    "Interface with {} endpoints, class {}, subclass {}, protocol {}."_sv,
                                    interface.endpoints.size(), interface.interface_class,
                                    interface.interface_subclass, interface.interface_protocol);
                                for (auto& ep : interface.endpoints) {
                                    DBG::dbgln("    Endpoint {}: {} {}, max packets {}."_sv,
                                               ep.number, ep.ttype, ep.direction,
                                               ep.max_packet_size);
                                    if (interface.interface_alternative == 0) {
                                        configuration_endpoints.push_back(ep);
                                    }
                                }
                            }

                            DBG::dbgln("Enabling configuration {}."_sv, configuration.config_value);
                            device.enable_configuration(
                                configuration.config_value, configuration_endpoints,
                                bek::function<void(bool)>(
                                    [&device, interfaces = bek::move(interfaces)](bool success) {
                                        VERIFY(success);
                                        for (auto& interface : interfaces) {
                                            if (interface.interface_alternative == 0) {
                                                probe_device(device, interface);
                                            }
                                        }
                                    }));
                        },
                        device));
                },
                device));
        },
        device));
}

usb::Registrar& usb::Registrar::the() {
    static auto* registrar = new Registrar;
    return *registrar;
}

void usb::Registrar::register_device(usb::Device& device) {
    m_devices.push_back(&device);
    begin_enumeration(device);
}

void usb::bek_basic_format(bek::OutputStream& out, const TransferType& t) {
    switch (t) {
        case TransferType::Control:
            out.write("control"_sv);
            break;
        case TransferType::Isochronous:
            out.write("isochronous"_sv);
            break;
        case TransferType::Bulk:
            out.write("bulk"_sv);
            break;
        case TransferType::Interrupt:
            out.write("interrupt"_sv);
            break;
    }
}
void usb::bek_basic_format(bek::OutputStream& out, const Direction& d) {
    switch (d) {
        case Direction::In:
            out.write("in"_sv);
            break;
        case Direction::Out:
            out.write("out"_sv);
            break;
    }
}
