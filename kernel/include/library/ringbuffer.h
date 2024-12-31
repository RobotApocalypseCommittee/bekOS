// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024 Bekos Contributors
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

#ifndef BEKOS_RINGBUFFER_H
#define BEKOS_RINGBUFFER_H

#include "bek/types.h"
#include "bek/vector.h"
#include "transactional_buffer.h"

class ring_buffer {
public:
    ring_buffer(uSize size);
    /// Always non-blocking.
    expected<uSize> read_to(TransactionalBuffer& buffer, bool partial);
    expected<uSize> write_to(TransactionalBuffer& buffer, bool partial);

    uSize pending_bytes() const;
    uSize free_bytes() const;
    uSize capacity() const { return m_buffer.size(); }

private:
    bek::vector<u8> m_buffer;
    uSize m_read_idx;
    uSize m_write_idx;
};

#endif  // BEKOS_RINGBUFFER_H
