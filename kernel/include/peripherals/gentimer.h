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

#ifndef BEKOS_GENTIMER_H
#define BEKOS_GENTIMER_H

#include "bek/types.h"
#include "device.h"
#include "interrupt_controller.h"
#include "library/function.h"

class TimerDevice : public Device {
public:
    enum class CallbackAction { Cancel, Reschedule };

    Kind kind() const override { return Device::Kind::Timer; }
    UserspaceProtocol userspace_protocol() const override { return UserspaceProtocol::None; }

    virtual u64 get_frequency()                                                 = 0;
    virtual u64 get_ticks()                                                     = 0;
    virtual bool schedule_callback(bek::function<CallbackAction()>, long ticks) = 0;
};

unsigned long getClockFrequency();
unsigned long getClockTicks();

/// Returns difference between times in microseconds
unsigned long getTimeDifference(u64 tickcount1, u64 tickcount2);
void bad_udelay(unsigned int microseconds);

#endif //BEKOS_GENTIMER_H
