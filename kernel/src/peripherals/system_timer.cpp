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

#include "peripherals/system_timer.h"
#include <peripherals/peripherals.h>
#include <printf.h>
#include <interrupts/int_ctrl.h>

#define TIMER_BASE (PERIPHERAL_BASE + 0x3000)
#define TIMER_CS (TIMER_BASE)
#define TIMER_CLO (TIMER_BASE + 0x4)
#define TIMER_CHI (TIMER_BASE + 0x8)
#define TIMER_C1 (TIMER_BASE + 0x10)
#define TIMER_C3 (TIMER_BASE + 0x18)

#define TIMER_FREQUENCY 1000000

template<int n>
system_timer<n>::system_timer(): m_handler(this) {
    check_and_clear_interrupt();
}

template<int n>
void system_timer<n>::set_interval(unsigned long microseconds) {
    interval = microseconds * TIMER_FREQUENCY / 1000000;
    unsigned long cnt = get_count();
    mmio_write(count_reg, static_cast<unsigned int>(cnt) + interval);
}

template<int n>
unsigned long system_timer<n>::get_count() {
    unsigned int chi = mmio_read(TIMER_CHI);
    unsigned int clo = mmio_read(TIMER_CLO);

    while (mmio_read(TIMER_CHI) != chi) {
        chi = mmio_read(TIMER_CHI);
        clo = mmio_read(TIMER_CHI);
    }
    return ((unsigned long) chi << 32u) | clo;
}

template<int n>
bool system_timer<n>::check_and_clear_interrupt() {
    unsigned int ans = mmio_read(TIMER_CS);
    if ((ans & (1u << n)) != 0) {
        mmio_write(TIMER_CS, 1u << n);
        return true;
    }
    return false;
}

template<int n>
system_timer<n>::int_handler::int_handler(system_timer<n>* mTimer):m_timer(mTimer) {}

template<int n>
bool system_timer<n>::int_handler::trigger() {
    return m_timer->handle_event();
}

template<>
const uintptr_t system_timer<1>::count_reg = TIMER_C1;

template<int n>
bcm_interrupt_handler* system_timer<n>::getHandler() {
    return &m_handler;
}

template<int n>
bool system_timer<n>::handle_event() {
    if (check_and_clear_interrupt()) {
        unsigned long cnt = get_count();
        mmio_write(count_reg, static_cast<unsigned int>(cnt) + interval);
        enable_interrupts();
        m_userhandler();
        disable_interrupts();
        return true;
    }
    return false;
}

template<int n>
void system_timer<n>::setTickHandler(void (* fn)()) {
    m_userhandler = fn;
}

template<>
const uintptr_t system_timer<3>::count_reg = TIMER_C3;

template class system_timer<1>;
template class system_timer<3>;
