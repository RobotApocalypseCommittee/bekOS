/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2025 Bekos Contributors
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

#ifndef BEKOS_MOUSE_H
#define BEKOS_MOUSE_H

#include "api/protocols/mouse.h"
#include "device.h"
#include "timer.h"

class MouseDevice : public Device {
public:

    Kind kind() const override { return Device::Kind::Mouse; }
    bek::str_view preferred_name_prefix() const override { return "generic.mouse"_sv; }

    bek::optional<DeviceProtocol> userspace_protocol() const override { return DeviceProtocol::Mouse; }
    expected<long> on_userspace_message(u64 id, TransactionalBuffer& message) override {
        auto msg_id = EXPECTED_TRY(message.read_object<bek::underlying_type<protocols::mouse::MessageKind>>());
        if (msg_id != protocols::mouse::GetReport) return EINVAL;
        if (message.size() != sizeof(protocols::mouse::GetReportMessage)) return EINVAL;
        protocols::mouse::GetReportMessage msg = {protocols::mouse::MessageKind::GetReport, fetch_report()};
        return message.write_from(&msg, sizeof(msg), 0).map_value([](auto v) { return static_cast<long>(v); });
    }

protected:
    protocols::mouse::Report fetch_report() {
        auto report = m_report;
        m_report = protocols::mouse::Report {
            .buttons = report.buttons,
            .delta_x = 0,
            .delta_y = 0,
            .sequence_number = static_cast<unsigned char>(report.sequence_number + 1),
        };
        return report;
    }

    void update_report(u8 buttons, i8 delta_x, i8 delta_y) {
        m_report.buttons = static_cast<decltype(protocols::mouse::Report::buttons)>(buttons);
        m_report.delta_x += delta_x;
        m_report.delta_y += delta_y;
    }
    protocols::mouse::Report m_report{};
private:
    using DeviceType = MouseDevice;
    static constexpr Device::Kind DeviceKind = Device::Kind::Mouse;

};

#endif //BEKOS_MOUSE_H
