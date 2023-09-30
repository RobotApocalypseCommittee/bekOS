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

#ifndef BEKOS_DEVICE_H
#define BEKOS_DEVICE_H

#include <library/traits.h>

class Device {
public:
    enum class Kind { Clock, UART, PCIeHost, InterruptController };

    virtual Kind kind() const = 0;

    virtual ~Device() = default;

    template <typename T>
        requires bek::same_as<T, typename T::DeviceType>
    static T* as(Device* device) {
        if (device->kind() == T::DeviceKind) {
            return static_cast<T*>(device);
        } else {
            return nullptr;
        }
    }
};

#endif  // BEKOS_DEVICE_H
