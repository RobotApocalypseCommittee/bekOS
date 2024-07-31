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

#include "bek/traits.h"
#include "bek/vector.h"
#include "library/intrusive_shared_ptr.h"
#include "library/iteration_decision.h"

enum class UserspaceProtocol { None, CharacterStream, Framebuffer };

class Device : public bek::RefCounted<Device> {
public:
    enum class Kind { Clock, UART, PCIeHost, InterruptController, Framebuffer, Timer };

    [[nodiscard]] virtual Kind kind() const = 0;
    [[nodiscard]] virtual UserspaceProtocol userspace_protocol() const {
        return UserspaceProtocol::None;
    };
    virtual void on_userspace_message(u32 process_id, bek::mut_buffer message);

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
void Device::on_userspace_message(u32 process_id, bek::mut_buffer message) { ASSERT_UNREACHABLE(); }

class DeviceRegistry {
public:
    static DeviceRegistry& the();

    void register_device(bek::shared_ptr<Device> device);

    template <typename F>
    void for_each_of_type(F fn, Device::Kind kind) {
        for (auto& dev : m_devices) {
            if (kind == dev->kind()) {
                if (fn(dev) == IterationDecision::Break) {
                    break;
                }
            }
        }
    }

private:
    bek::vector<bek::shared_ptr<Device>> m_devices;
};

#endif  // BEKOS_DEVICE_H
