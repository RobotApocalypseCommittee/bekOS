// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "core/device.h"

#include "core/syscall.h"

bek::vector<core::Device> core::Device::get_devices(DeviceProtocol protocol_filter) {
    bek::vector<u8> buffer(1000);

    if (auto res = core::syscall::list_devices(buffer.data(), buffer.size(), protocol_filter); res != ESUCCESS) {
        if (res != EOVERFLOW) return {};
        while (res == EOVERFLOW) {
            buffer = bek::vector<u8>(buffer.size() * 2);
            res = core::syscall::list_devices(buffer.data(), buffer.size(), protocol_filter);
        }
    }

    bek::vector<Device> result;
    // Now parse buffer.
    uSize entry_offset = 0;
    while (entry_offset < buffer.size()) {
        auto& entry = *reinterpret_cast<const sc::DeviceListItem*>(buffer.data() + entry_offset);
        if (entry.protocol != DeviceProtocol::NoProtocol) {
            result.push_back(Device{
                .name = bek::string{entry.name()},
                .protocol = entry.protocol,
            });
        }
        if (entry.next_offset == 0) break;
        entry_offset += entry.next_offset;
    }
    return result;
}
bek::vector<core::Device> core::Device::get_devices() {
    bek::vector<u8> buffer(1000);

    if (auto res = core::syscall::list_devices(buffer.data(), buffer.size()); res != ESUCCESS) {
        if (res != EOVERFLOW) return {};
        while (res == EOVERFLOW) {
            buffer = bek::vector<u8>(buffer.size() * 2);
            res = core::syscall::list_devices(buffer.data(), buffer.size());
        }
    }

    bek::vector<Device> result;
    // Now parse buffer.
    uSize entry_offset = 0;
    while (entry_offset < buffer.size()) {
        auto& entry = *reinterpret_cast<const sc::DeviceListItem*>(buffer.data() + entry_offset);
        if (entry.protocol != DeviceProtocol::NoProtocol) {
            result.push_back(Device{
                .name = bek::string{entry.name()},
                .protocol = entry.protocol,
            });
        }
        if (entry.next_offset == 0) break;
        entry_offset += entry.next_offset;
    }
    return result;
}
