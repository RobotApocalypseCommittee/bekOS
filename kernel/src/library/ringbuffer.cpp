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

#include "library/ringbuffer.h"

ring_buffer::ring_buffer(uSize size): m_buffer(size), m_read_idx{}, m_write_idx{} {}

uSize ring_buffer::pending_bytes() const {
    return (m_write_idx >= m_read_idx) ? m_write_idx - m_read_idx : m_buffer.size() - m_read_idx + m_write_idx;
}
uSize ring_buffer::free_bytes() const {
    return (m_write_idx >= m_read_idx) ? m_buffer.size() - m_write_idx + m_read_idx - 1 : m_read_idx - m_write_idx - 1;
}
expected<uSize> ring_buffer::read_to(TransactionalBuffer& buffer, bool partial) {
    // It must be at least _possible_ to fulfil request!
    VERIFY(partial || buffer.size() < m_buffer.size());

    uSize read_space = pending_bytes();
    // TODO: Lock.

    if (!partial && read_space < buffer.size()) return EAGAIN;

    uSize bytes_read = 0;
    // This will loop at most twice!
    while (read_space && bytes_read < buffer.size()) {
        auto segment_read_space = (m_write_idx >= m_read_idx) ? m_write_idx - m_read_idx : m_buffer.size() - m_read_idx;
        auto to_read = bek::min(segment_read_space, buffer.size() - bytes_read);
        EXPECTED_TRY(buffer.write_from(m_buffer.data() + m_read_idx, to_read, bytes_read));
        bytes_read += to_read;
        m_read_idx += to_read;
        if (m_read_idx == m_buffer.size()) m_read_idx = 0;

        read_space = pending_bytes();
    }
    return bytes_read;
}
expected<uSize> ring_buffer::write_to(TransactionalBuffer& buffer, bool partial) {
    uSize write_space = free_bytes();
    if (write_space < buffer.size() && !partial) return EAGAIN;

    uSize bytes_written = 0;
    // This will loop at most twice.
    while (write_space && bytes_written < buffer.size()) {
        // Total space we can write contiguously.
        auto segment_write_space =
            (m_write_idx >= m_read_idx) ? m_buffer.size() - m_write_idx : m_read_idx - m_write_idx - 1;
        if (m_read_idx == 0) segment_write_space -= 1;

        auto to_write = bek::min(segment_write_space, buffer.size() - bytes_written);
        EXPECTED_TRY(buffer.read_to(m_buffer.data() + m_write_idx, to_write, bytes_written));
        bytes_written += to_write;
        m_write_idx += to_write;
        if (m_write_idx == m_buffer.size()) m_write_idx = 0;
        write_space = free_bytes();
    }
    return bytes_written;
}
