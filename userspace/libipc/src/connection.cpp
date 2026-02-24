// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025-2026 Bekos Contributors
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

#include "ipc/connection.h"

#include <core/io.h>
#include <core/syscall.h>

constexpr inline uSize BUFFER_SIZE = 1024;

ErrorCode ipc::Connection::send_message(Message& msg) {
    auto buf = msg.to_buffer();
    EXPECTED_TRY(core::syscall::interlink::send(m_fd, buf.header()));
    return ESUCCESS;
}
ErrorCode ipc::Connection::poll() {
    MessageBuffer message_buffer{bek::vector<u8>(BUFFER_SIZE)};
    auto n_result = core::syscall::interlink::receive(m_fd, &message_buffer.header(), BUFFER_SIZE);
    if (n_result.has_error()) {
        if (n_result.error() == EAGAIN) return ESUCCESS;
        return n_result.error();
    }
    auto n = n_result.value();
    if (n) {
        if (!message_buffer.verify()) return EINVAL;
        auto msg = EXPECTED_TRY(Message::from_buffer(message_buffer));
        core::fprintln(core::stdout, "Received message: {}, with data of {} bytes"_sv, msg.message_id(),
                       msg.data_size());
        return dispatch_message(msg.message_id(), msg);
    } else {
        return ESUCCESS;
    }
}