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

#ifndef BEKOS_USB_DESCRIPTORS_H
#define BEKOS_USB_DESCRIPTORS_H

#include <library/string.h>

#include "library/buffer.h"
#include "library/vector.h"
#include "usb.h"
namespace usb {

struct DescriptorBase {
    enum DescriptorType : u8 {
        Device        = 0x01,
        Configuration = 0x02,
        String        = 0x03,
        Interface     = 0x04,
        Endpoint      = 0x05,

        Hid       = 0x21,
        HidReport = 0x22,
    };
    u8 length;
    DescriptorType kind;
};

struct DeviceDescriptor : DescriptorBase {
    u16 version_bcd;
    u8 device_class;
    u8 device_subclass;
    u8 device_protocol;
    u8 max_packet_size;
    u16 vendor_id;
    u16 product_id;
    u16 release_bcd;
    u8 manufacturer_string;
    u8 product_string;
    u8 serial_string;
    u8 configuration_count;
};
static_assert(sizeof(DeviceDescriptor) == 18);

/*struct QualifierDescriptor: DescriptorBase {
public:
    u16 version_bcd;
    u8 device_class;
    u8 device_subclass;
    u8 device_protocol;
    u8 max_packet_size;
    u8 configuration_num;
};*/

struct [[gnu::packed]] ConfigurationDescriptor : DescriptorBase {
    u16 total_length;
    u8 num_interfaces;
    u8 config_value;
    u8 config_string;
    u8 attributes;
    /// In units of 2mA
    u8 max_power;
};

static_assert(sizeof(ConfigurationDescriptor) == 9);

struct InterfaceDescriptor : DescriptorBase {
    u8 number;
    u8 alternate_setting;
    u8 num_endpoints;
    u8 interface_class;
    u8 interface_subclass;
    u8 interface_protocol;
    u8 interface_string;
};

static_assert(sizeof(InterfaceDescriptor) == 9);

struct [[gnu::packed]] EndpointDescriptor : DescriptorBase {
    u8 endpoint_address;
    u8 attributes;
    // TODO: isochronous endpoint
    u16 max_packet_size;
    u8 interval;

    [[nodiscard]] usb::Endpoint to_endpoint() const {
        return {
            static_cast<u8>(endpoint_address & 0xF),
            (endpoint_address & (1u << 7)) ? Direction::In : Direction::Out,
            static_cast<TransferType>(attributes & 0b11),
            // FIXME: USB2 (but not 3 std?) says bits 11:12 in high speed devices is burst value.
            max_packet_size, interval, static_cast<bool>(attributes & (1u << 4))};
    }
};
static_assert(sizeof(EndpointDescriptor) == 7);

bek::vector<Interface> parse_configuration(bek::buffer data);

}

#endif // BEKOS_USB_DESCRIPTORS_H

