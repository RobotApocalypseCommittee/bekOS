/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2021  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_USB_DESCRIPTORS_H
#define BEKOS_USB_DESCRIPTORS_H

#include <library/string.h>

#include "library/vector.h"
#include "transfers.h"
namespace usb {

class ConfigurationDescriptor;
class InterfaceDescriptor;
class EndpointDescriptor;

class Descriptor {

};

class DeviceDescriptor: public Descriptor {
public:
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
    bek::vector<ConfigurationDescriptor> configurations;
};

class QualifierDescriptor: public Descriptor {
public:
    u16 version_bcd;
    u8 device_class;
    u8 device_subclass;
    u8 device_protocol;
    u8 max_packet_size;
    u8 configuration_num;
};

class ConfigurationDescriptor: public Descriptor {
public:
    u8 config_value;
    u8 config_string;
    bool self_powered;
    bool remote_wakeup;
    /// In units of 2mA
    u8 max_power;
    bek::vector<InterfaceDescriptor> interfaces;
};

class InterfaceDescriptor: public Descriptor {
public:
    u8 number;
    u8 alternate_setting;
    u8 interface_class;
    u8 interface_subclass;
    u8 interface_protocol;
    u8 interface_string;
    bek::vector<EndpointDescriptor> endpoints;
};

class EndpointDescriptor: public Descriptor {
public:
    u8 endpoint_number;
    TransferDirection direction;
    TransferType type;
    // TODO: isochronous endpoint
    u16 max_packet_size;
    /// Default: 1
    u8 transactions_per_microframe;
    u8 polling_interval;
};

class StringDescriptor: public Descriptor {
    bek::string string;
};

class HIDDescriptor: public Descriptor {

};

}

#endif // BEKOS_USB_DESCRIPTORS_H

