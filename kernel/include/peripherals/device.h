/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024-2025 Bekos Contributors
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

#include "api/device_protocols.h"
#include "bek/optional.h"
#include "bek/traits.h"
#include "library/hashtable.h"
#include <bek/intrusive_shared_ptr.h>
#include "library/iteration_decision.h"
#include "library/user_buffer.h"
#include "process/entity.h"

class Device : public bek::RefCounted<Device> {
public:
    enum class Kind { Clock, UART, PCIeHost, InterruptController, Framebuffer, Timer, Keyboard, Mouse };

    [[nodiscard]] virtual Kind kind() const = 0;
    [[nodiscard]] virtual bek::optional<DeviceProtocol> userspace_protocol() const { return {}; };
    [[nodiscard]] virtual bek::str_view preferred_name_prefix() const = 0;

    virtual expected<long> on_userspace_message(u64 id, TransactionalBuffer& message) {
        (void)id;
        (void)message;
        ASSERT_UNREACHABLE();
    }

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

class DeviceHandle : public EntityHandle {
public:
    explicit DeviceHandle(bek::shared_ptr<Device> device) : m_device(bek::move(device)) {}
    Kind kind() const override { return EntityHandle::Kind::Device; }
    SupportedOperations get_supported_operations() const override { return Message; }
    expected<long> message(u64 id, TransactionalBuffer& buffer) const override {
        return m_device->on_userspace_message(id, buffer);
    }

private:
    bek::shared_ptr<Device> m_device;
};

class DeviceRegistry {
public:
    static DeviceRegistry& the();

    bek::str_view register_device(bek::str_view name_prefix, bek::shared_ptr<Device> device);

    bek::shared_ptr<Device> get(bek::str_view name) const;
    bek::shared_ptr<DeviceHandle> open(bek::str_view name) const;

    template <bek::callable_with_args<IterationDecision, bek::str_view, bek::shared_ptr<Device>&> F>
    void for_each_device(F fn) {
        for (auto& p : m_devices) {
            if (fn(p.first.view(), p.second) == IterationDecision::Break) {
                break;
            }
        }
    }

private:
    bek::hashtable<bek::string, bek::shared_ptr<Device>> m_devices{};
};

#endif  // BEKOS_DEVICE_H
