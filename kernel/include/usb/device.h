/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
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


#ifndef BEKOS_DEVICE_H
#define BEKOS_DEVICE_H

namespace usb {
class Device {
public:
    enum DeviceStatus {
        Attached,
        Powered,
        Default,
        Address,
        Configured
    };
    DeviceStatus get_status() const;
    bool is_suspended() const;
    int get_address() const;

    bool enumerate_device();

    uSize functionality_count() const;
    Functionality& get_functionality(uSize i) const;

    HostController& get_host_controller() const;

private:
    HostController* m_controller;
    DeviceStatus m_status;
    bool is_suspended;
    int m_address;

};
}


#endif  // BEKOS_DEVICE_H
