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

#include "core/process.h"

#include "core/syscall.h"
#include "cxxabi.h"

extern "C" {
[[gnu::weak]] extern void _fini();
[[gnu::weak]] extern void (*const __fini_array_start)();  // NOLINT(*-reserved-identifier)
[[gnu::weak]] extern void (*const __fini_array_end)();    // NOLINT(*-reserved-identifier)
}

void core::exit(int exit_code) {
    __cxa_finalize(nullptr);

    using uPtr = unsigned long;
    uPtr init_ptr = reinterpret_cast<uPtr>(&__fini_array_start);
    for (; init_ptr < reinterpret_cast<uPtr>(&__fini_array_end); init_ptr += sizeof(void (*)())) {
        (*(void (**)(void))init_ptr)();
    }
    _fini();

    core::syscall::exit(exit_code);
}
