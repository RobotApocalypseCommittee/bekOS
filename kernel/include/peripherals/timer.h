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

#ifndef BEKOS_TIMER_H
#define BEKOS_TIMER_H

#include "device.h"
#include "library/function.h"

class TimerDevice : public Device {
public:
    struct CallbackAction {
        static const CallbackAction Cancel;
        static constexpr CallbackAction Reschedule(long ticks) { return CallbackAction{ticks}; }
        bool is_cancel() const { return period < -1; }
        bool is_reschedule() const { return period >= 0; }
        long period;
    };

    Kind kind() const override { return Device::Kind::Timer; }
    bek::str_view preferred_name_prefix() const override { return "generic.timer"_sv; }

    virtual u64 get_frequency() = 0;
    virtual u64 get_ticks() = 0;
    virtual bool schedule_callback(bek::function<CallbackAction()>, long ticks) = 0;

    using DeviceType = TimerDevice;
    static constexpr Kind DeviceKind = Device::Kind::Timer;
};
inline constexpr TimerDevice::CallbackAction TimerDevice::CallbackAction::Cancel = CallbackAction{-1};

namespace timing {
ErrorCode initialise();
ErrorCode schedule_callback(bek::function<TimerDevice::CallbackAction(u64)> action, long nanoseconds);

uSize nanoseconds_since_start();

constexpr u64 nanoseconds_from_frequency(u64 hertz) {
    return 1'000'000'000 / hertz;
}

void spindelay_us(uSize microseconds);

}  // namespace timing

#endif  // BEKOS_TIMER_H
