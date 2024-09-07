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

#include "core/io.h"
#include "core/process.h"

[[noreturn]] void bek::panic() {
    core::fprintln(core::stderr, "PANIC!"_sv);
    core::exit(1);
}
[[noreturn]] void bek::panic(const char* message, const char* file_name, unsigned int line) {
    core::fprintln(core::stderr, "PANIC: {} in {} line {}."_sv, message, file_name, line);
    core::exit(1);
}
[[noreturn]] void bek::assertion_failed(const char* pExpr, const char* pFile, unsigned nLine) {
    core::fprintln(core::stderr, "Assertion Failed: {} in {} line {}."_sv, pExpr, pFile, nLine);
    core::exit(1);
}
