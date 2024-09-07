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

#include "core/error.h"
#include "core/io.h"
#include "core/syscall.h"
#include "library/hashtable.h"

constexpr inline bool LOCAL_ECHO = true;

using CmdFn = core::expected<bool> (*)(bek::vector<bek::str_view>& command);

bek::hashtable<bek::str_view, CmdFn> builtin_commands;

core::expected<bek::string> prompt_for_command() {
    core::fprint(core::stdout, "> "_sv);
    bek::StringStream ss;
    char c;

    while (auto n = EXPECTED_TRY(core::stdin.read(&c, 1))) {
        if (!LOCAL_ECHO) {
            core::fprint(core::stdout, "{}"_sv, c);
        }
        if (c == '\n') {
            return ss.build_and_clear();
        }
        ss.write(c);
    }

    // Reached EOF.
    core::fprintln(core::stdout, "End of Text"_sv);
    return bek::string{"quit"};
}

bek::expected<bek::vector<bek::str_view>, bek::string> parse_command(bek::string& str) {
    bek::vector<bek::str_view> components;

    u32 start_idx = 0;
    for (u32 i = 0; i < str.size(); i++) {
        if (str.data()[i] == ' ') {
            // Trim if excessive whitespace.
            if (i == start_idx) {
                start_idx++;
            } else {
                components.push_back(bek::str_view{&str.data()[start_idx], i - start_idx});
            }
        }
    }
    return components;
}

core::expected<bool> dispatch_command(bek::vector<bek::str_view>& command) {
    if (command.size() == 0) {
        return false;
    }

    if (auto builtin = builtin_commands.find(command[0]); builtin) {
        return (**builtin)(command);
    }

    // TODO: Fork.
    core::fprintln(core::stderr, "Unknown command: {}"_sv, command[0]);
    return false;
}

ErrorCode loop() {
    bool should_quit = false;
    while (!should_quit) {
        auto cmd = EXPECTED_TRY(prompt_for_command());

        auto parse_res = parse_command(cmd);
        if (parse_res.has_error()) {
            core::fprintln(core::stderr, "Invalid command: {}."_sv, parse_res.error().view());
            continue;
        }

        should_quit = EXPECTED_TRY(dispatch_command(parse_res.value()));
    }
    return ESUCCESS;
}

core::expected<bool> builtin_quit(bek::vector<bek::str_view>& command) {
    if (command.size() > 1) {
        core::fprintln(core::stderr, "sh: warn: {} takes no arguments."_sv, command[0]);
    }
    return true;
}

core::expected<bool> builtin_help(bek::vector<bek::str_view>& command) {
    if (command.size() > 1) {
        core::fprintln(core::stderr, "sh: warn: {} takes no arguments."_sv, command[0]);
    }
    core::fprintln(core::stdout, "BekOS Shell. Built-in commands:"_sv);
    for (auto [key, _] : builtin_commands) {
        core::fprintln(core::stdout, "    {}"_sv, key);
    }
    return false;
}

int main(int argc, char** argv) {
    auto pid = core::syscall::get_pid();
    if (pid.has_error()) return pid.error();
    core::fprintln(core::stdout, "BekOS Shell, version 0.1, pid {}."_sv, pid.value());

    // Setup Builtins.
    builtin_commands.insert({"q"_sv, builtin_quit});
    builtin_commands.insert({"quit"_sv, builtin_quit});
    builtin_commands.insert({"help"_sv, builtin_help});

    return loop();
}