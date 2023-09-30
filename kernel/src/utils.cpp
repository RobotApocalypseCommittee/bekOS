/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#include "utils.h"

#include "library/assertions.h"

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

namespace __cxxabiv1 {
/* guard variables */

/* The ABI requires a 64-bit type.  */
__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release(__guard *);
extern "C" void __cxa_guard_abort(__guard *);

extern "C" int __cxa_guard_acquire(__guard *g) { return !*(char *)(g); }

extern "C" void __cxa_guard_release(__guard *g) { *(char *)g = 1; }

extern "C" void __cxa_guard_abort(__guard *) {}
}  // namespace __cxxabiv1

void bek::panic() {
    while (true) {
        asm volatile("nop");
    }
}
