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

#ifndef BEKOS_GENTIMER_H
#define BEKOS_GENTIMER_H

#include "bek/types.h"
#include "library/function.h"
#include "peripherals//interrupt_controller.h"
#include "peripherals//timer.h"
#include "peripherals/device.h"

class ArmGenericTimer : public TimerDevice {
public:
    static bek::shared_ptr<TimerDevice> probe_timer(InterruptController& global_intc);
    ArmGenericTimer();

    u64 get_frequency() override;
    u64 get_ticks() override;
    bool schedule_callback(bek::function<CallbackAction()> function, long ticks) override;

private:
    void on_trigger();
    bek::function<CallbackAction(), false, true> m_callback;
};

#endif //BEKOS_GENTIMER_H
