/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2025-2026 Bekos Contributors
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

#include <bek/str.h>
#include <core/io.h>
#include <core/syscall.h>

template <typename... Args>
void dbgln(bek::str_view view, Args&&... args) {
    core::fprintln(core::stdout, view, args...);
}

core::expected<long> launch_program(bek::str_view path) {
    core::stdout.flush();
    auto fork_result = EXPECTED_TRY(core::syscall::fork());
    if (fork_result == 0) {
        auto exec_result = core::syscall::exec(path, {}, {});
        if (exec_result.has_error()) {
            dbgln("Exec {} failed: {}"_sv, path, exec_result.error());
        }
        core::syscall::exit(-1);
    }
    dbgln("Launched {} with pid {}"_sv, path, fork_result);
    return fork_result;
}

int main() {
    dbgln("wininit: starting windowing system..."_sv);

    auto ws_pid = launch_program("/bin/windowserver"_sv);
    if (ws_pid.has_error()) {
        dbgln("Failed to launch windowserver: {}"_sv, ws_pid.error());
        return -1;
    }

    // Wait for windowserver to advertise its service.
    core::syscall::sleep(500'000);

    dbgln("wininit: starting about executable..."_sv);
    auto about_pid = launch_program("/bin/about"_sv);
    if (about_pid.has_error()) {
        dbgln("Failed to launch about: {}"_sv, about_pid.error());
        return -1;
    }

    // Idle — child processes are running.
    while (true) {
        core::syscall::sleep(1'000'000);
    }
    return 0;
}
