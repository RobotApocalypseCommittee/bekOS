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

#ifndef BEKOS_CORE_DEVICE_H
#define BEKOS_CORE_DEVICE_H

#include "api/device_protocols.h"
#include "bek/str.h"
#include "bek/types.h"
#include "bek/vector.h"

namespace core {

struct Device {
    bek::string name;
    DeviceProtocol protocol;

    static bek::vector<Device> get_devices(DeviceProtocol protocol_filter);
    static bek::vector<Device> get_devices();
};

}  // namespace core

#endif  // BEKOS_CORE_DEVICE_H
