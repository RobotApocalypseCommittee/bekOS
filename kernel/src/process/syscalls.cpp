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

#include <printf.h>
#include <peripherals/gentimer.h>
#include <process/process.h>

extern "C"
void syscall_uart_write(char* str) {
    printf(str);
}

extern "C"
void syscall_delay_us(unsigned long us) {
    bad_udelay(us);
}

extern "C"
int syscall_get_pid() {
    return processManager.getCurrentProcess()->processID;
}

extern "C"
void * const syscall_table[] = {reinterpret_cast<void*>(syscall_uart_write), reinterpret_cast<void*>(syscall_delay_us), reinterpret_cast<void*>(syscall_get_pid)};
