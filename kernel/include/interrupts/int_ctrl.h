/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024-2026 Bekos Contributors
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

#ifndef BEKOS_INT_CTRL_H
#define BEKOS_INT_CTRL_H

extern "C" void do_set_vector_table(void);

extern "C"
void enable_interrupts(void);

extern "C"
void disable_interrupts(void);

extern "C" unsigned char save_and_disable_interrupts(void);

extern "C" void restore_interrupts(unsigned char flags);

struct InterruptDisabler {
    InterruptDisabler(): m_state{save_and_disable_interrupts()} { }
    ~InterruptDisabler() { restore_interrupts(m_state); }
    unsigned char m_state;
};

#endif //BEKOS_INT_CTRL_H
