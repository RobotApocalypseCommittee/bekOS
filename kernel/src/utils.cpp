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

#include <memory_locations.h>
#include <kstring.h>


#include "utils.h"

unsigned long phys_to_virt(unsigned long physical_address) {
    return physical_address + VA_START;
}

unsigned long virt_to_phys(unsigned long virtual_address) {
    return virtual_address - VA_START;
}

unsigned long next_power_2(unsigned long x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

long round_up(long n, long multiple) {
    int remainder = n % multiple;
    if (remainder == 0)
        return n;

    return n + multiple - remainder;
}

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
}
