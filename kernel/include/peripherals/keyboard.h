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

#ifndef BEKOS_KEYBOARD_H
#define BEKOS_KEYBOARD_H

#include "api/protocols/kb.h"
#include "device.h"

class KeyboardDevice : public Device {
public:
    virtual protocols::kb::Report get_report() const = 0;

    Kind kind() const override { return Device::Kind::Keyboard; }
    bek::str_view preferred_name_prefix() const override { return "generic.keyboard"_sv; }

    bek::optional<DeviceProtocol> userspace_protocol() const override { return DeviceProtocol::Keyboard; }
    expected<long> on_userspace_message(u64 id, TransactionalBuffer& message) override {
        auto msg_id = EXPECTED_TRY(message.read_object<bek::underlying_type<protocols::kb::MessageKind>>());
        if (msg_id != protocols::kb::MessageKind::GetReport) return EINVAL;
        if (message.size() != sizeof(protocols::kb::GetReportMessage)) return EINVAL;
        protocols::kb::GetReportMessage msg = {protocols::kb::MessageKind::GetReport, get_report()};
        return message.write_from(&msg, sizeof(msg), 0).map_value([](auto v) { return static_cast<long>(v); });
    }

private:
    using DeviceType = KeyboardDevice;
    static constexpr Device::Kind DeviceKind = Device::Kind::Keyboard;
};

#endif  // BEKOS_KEYBOARD_H
