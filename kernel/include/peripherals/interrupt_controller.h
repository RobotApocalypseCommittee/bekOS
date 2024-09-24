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

#ifndef BEKOS_INTERRUPT_CONTROLLER_H
#define BEKOS_INTERRUPT_CONTROLLER_H

#include <library/function.h>

#include "bek/buffer.h"
#include "bek/optional.h"
#include "device.h"
#include "mm/areas.h"

using InterruptHandler = bek::function<void(void), false, false>;

class InterruptController : public Device {
public:
    using DeviceType                 = InterruptController;
    static constexpr Kind DeviceKind = Device::Kind::InterruptController;

    struct Handle {
        InterruptController* controller{nullptr};
        u32 interrupt_id{0};

        void register_handler(InterruptHandler&& handler) {
            controller->register_handler(interrupt_id, bek::move(handler));
        }
        void enable() { controller->enable_interrupt(interrupt_id); }
        void disable() { controller->disable_interrupt(interrupt_id); }
        explicit operator bool() const { return controller; }
    };

    virtual Handle register_interrupt(bek::buffer selection_data)               = 0;
    virtual void register_handler(u32 interrupt_id, InterruptHandler&& handler) = 0;
    virtual void enable_interrupt(u32 interrupt_id)                             = 0;
    virtual void disable_interrupt(u32 interrupt_id)                            = 0;
    virtual void handle_interrupt()                                             = 0;
    Kind kind() const override;
    bek::str_view preferred_name_prefix() const override;
};

class LegacyInterruptController {
public:
    enum interrupt_type {
        SYSTEM_TIMER_1 = 1,
        SYSTEM_TIMER_3 = 3,
        USB            = 9,
        ARM_TIMER      = 64,
        NONE           = 255
    };
    void enable(interrupt_type t);
    void disable(interrupt_type t);

    void register_handler(interrupt_type interruptType, InterruptHandler&& handler);

    bool handle();

private:
    bek::optional<InterruptHandler> handlers[96] = {};
    mem::DeviceArea m_area;
};

#endif //BEKOS_INTERRUPT_CONTROLLER_H
