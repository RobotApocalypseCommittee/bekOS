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

#include "core/io.h"
#include "core/syscall.h"

int main(int argc, char** argv) {
    auto pid = core::syscall::get_pid();
    if (pid.has_error()) return pid.error();
    core::fprintln(core::stdout, "BekOS Stub, version 0.1, pid {}."_sv, pid.value());
    core::fprint(core::stdout, "Called with args: "_sv);
    for (int i = 0; i < argc; i++) {
        core::fprint(core::stdout, "{} "_sv, bek::str_view{argv[i]});
    }
    core::fprintln(core::stdout, ""_sv);
    return 0;
}