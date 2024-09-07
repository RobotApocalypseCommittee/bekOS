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

#include "bek/assertions.h"
#include "core/process.h"
#include "cxxabi.h"

inline constexpr unsigned long ATEXIT_MAXIMUM_FUNCTIONS = 32;
struct AtExitFuncEntry {
    void (*func)(void*);
    void* obj_ptr;
    void* dso_handle;
};
unsigned long core_atexit_function_count = 0;
AtExitFuncEntry core_atexit_functions[ATEXIT_MAXIMUM_FUNCTIONS];

extern "C" {
[[gnu::naked, noreturn, gnu::used]] void _start(int, char**);
int main(int, char**);

[[gnu::weak]] extern void (*const __init_array_start)();  // NOLINT(*-reserved-identifier)
[[gnu::weak]] extern void (*const __init_array_end)();    // NOLINT(*-reserved-identifier)
[[gnu::weak]] extern void _init();

void __cxa_pure_virtual() {  // NOLINT(*-reserved-identifier)
    PANIC("Pure Virtual Method Called.");
}

[[maybe_unused]] [[maybe_unused]] void* __dso_handle = nullptr;
}  // extern "C"

extern "C" [[gnu::used, gnu::weak]] void _entry(int argc, char** argv) {
    _init();
    using uPtr = unsigned long;
    uPtr init_ptr = reinterpret_cast<uPtr>(&__init_array_start);
    for (; init_ptr < reinterpret_cast<uPtr>(&__init_array_end); init_ptr += sizeof(void (*)())) {
        (*(void (**)(void))init_ptr)();
    }

    int status = main(argc, argv);

    core::exit(status);
}

extern "C" int __cxa_atexit(void (*destructor)(void*), void* arg, void* dso) {
    if (core_atexit_function_count >= ATEXIT_MAXIMUM_FUNCTIONS) {
        return -1;
    }
    core_atexit_functions[core_atexit_function_count++] = {.func = destructor, .obj_ptr = arg, .dso_handle = dso};
    return 0;
}

extern "C" void __cxa_finalize(void* f) {
    for (auto i = core_atexit_function_count; i > 0; i--) {
        auto& entry = core_atexit_functions[i - 1];
        if (!entry.func || (f && entry.func != f)) {
            continue;
        }

        (*entry.func)(entry.obj_ptr);
    }
}
