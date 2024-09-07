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

#include "process/tty.h"

#include "library/debug.h"
#include "process/process.h"

using DBG = DebugScope<"TTY", true>;

expected<uSize> ProcessDebugSerial::write(u64 offset, TransactionalBuffer& buffer) {
    if (offset && offset != sc::INVALID_OFFSET_VAL) return ESPIPE;
    if (buffer.size() > 1024) return EFBIG;
    char buf[1024];
    EXPECTED_TRY(buffer.read_to(buf, buffer.size(), 0));

    bek::str_view str{buf, buffer.size()};
    auto proc_id = ProcessManager::the().current_process().pid();
    if (str[str.size() - 1] == '\n') {
        str = str.substr(0, str.size() - 1);
    }
    DBG::dbgln("({}) {}"_sv, proc_id, str);
    return buffer.size();
}
