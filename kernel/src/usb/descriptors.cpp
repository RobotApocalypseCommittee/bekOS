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

#include <usb/descriptors.h>

#include "library/debug.h"

using DBG = DebugScope<"USB", true>;

namespace usb {
bek::vector<Interface> parse_configuration(bek::buffer data) {
    auto& config = data.get_at<ConfigurationDescriptor>(0);
    // TODO: more
    VERIFY(config.num_interfaces == 1);
    VERIFY(config.length == sizeof(ConfigurationDescriptor));
    auto offset = sizeof(ConfigurationDescriptor);
    bek::vector<Interface> interfaces;
    while (offset < data.size()) {
        auto& header = data.get_at<DescriptorBase>(offset);
        switch (header.kind) {
            case DescriptorBase::Interface: {
                auto& int_descriptor = data.get_at<InterfaceDescriptor>(offset);
                interfaces.push_back({.interface_class       = int_descriptor.interface_class,
                                      .interface_subclass    = int_descriptor.interface_subclass,
                                      .interface_protocol    = int_descriptor.interface_protocol,
                                      .interface_number      = int_descriptor.number,
                                      .interface_alternative = int_descriptor.alternate_setting,
                                      .endpoints             = {}});
            } break;
            case DescriptorBase::Endpoint: {
                auto& ep_desc = data.get_at<EndpointDescriptor>(offset);
                interfaces.back().endpoints.push_back(ep_desc.to_endpoint());
            } break;
            default:
                DBG::dbgln("Unknown descriptor with type 0x{:x}."_sv, static_cast<u8>(header.kind));
                break;
        }
        VERIFY(header.length);
        offset += header.length;
    }
    return interfaces;
}

}  // namespace usb
