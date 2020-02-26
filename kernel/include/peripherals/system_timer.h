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

#ifndef BEKOS_SYSTEM_TIMER_H
#define BEKOS_SYSTEM_TIMER_H

#include <stdint.h>
#include "interrupt_controller.h"

template <int n>
class system_timer{
public:
    system_timer();

    void set_interval(unsigned long microseconds);

    bcm_interrupt_handler* getHandler();

    void setTickHandler(void (*fn)());

    system_timer(const system_timer&) = delete;
    system_timer& operator=(const system_timer&) = delete;

private:
    unsigned long get_count();
    bool check_and_clear_interrupt();
    unsigned long interval = 0;
    static const uintptr_t count_reg;

    void (*m_userhandler)();

    bool handle_event();

    class int_handler: public bcm_interrupt_handler {
    public:
        explicit int_handler(system_timer<n>* mTimer);
        bool trigger() override;
    private:
        system_timer<n>* m_timer;
    };

    int_handler m_handler;
};

#endif //BEKOS_SYSTEM_TIMER_H
