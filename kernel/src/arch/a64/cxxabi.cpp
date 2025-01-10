// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// NOLINTBEGIN(*-reserved-identifier)

extern "C" {
[[gnu::used]] void* __dso_handle = nullptr;


}

/// These are no-op for now - I'm assuming destructors don't need to run when system stops
extern "C" [[gnu::used]] int __cxa_atexit(void (*destructor)(void*), void* arg, void* dso) {
    (void)destructor;
    (void)arg;
    (void)dso;
    return 0;
}
extern "C" void __cxa_finalize(void*) {}

// NOLINTEND(*-reserved-identifier)