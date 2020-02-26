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

#ifndef BEKOS_INTERRUPT_CONTROLLER_H
#define BEKOS_INTERRUPT_CONTROLLER_H

class bcm_interrupt_handler {
public:
    virtual bool trigger() = 0;
};

class interrupt_controller {
public:
    enum interrupt_type {
        SYSTEM_TIMER_1 = 1,
        SYSTEM_TIMER_3 = 3,
        ARM_TIMER = 64,
        NONE = 255
    };
    void enable(interrupt_type t);
    void disable(interrupt_type t);

    void register_handler(interrupt_type interruptType, bcm_interrupt_handler* handler);

    bool handle();

private:
    bcm_interrupt_handler* handlers[96] = {};

};

#endif //BEKOS_INTERRUPT_CONTROLLER_H
