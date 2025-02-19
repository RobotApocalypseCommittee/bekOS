// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
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

#ifndef BEKOS_IPC_CONNECTION_H
#define BEKOS_IPC_CONNECTION_H
#include "message.h"

namespace ipc {

class Connection {
public:
    explicit Connection(long fd): m_fd(fd) {}
    virtual ~Connection() = default;
    ErrorCode poll();
protected:
    virtual ErrorCode dispatch_message(u32 id, Message& buffer) = 0;
    ErrorCode send_message(Message& msg);
private:
    long m_fd;
};

}

#endif //BEKOS_IPC_CONNECTION_H
