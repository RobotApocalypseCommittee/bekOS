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

#include "process/pipe.h"

inline constexpr uSize PIPE_DEFAULT_SIZE = 4096;
Pipe::Pipe() : m_data(PIPE_DEFAULT_SIZE), m_read_idx{}, m_write_idx{} {}

expected<uSize> Pipe::write(TransactionalBuffer& buffer, bool blocking) {
    // Total space available to write.
    auto write_space =
        (m_write_idx >= m_read_idx) ? m_data.size() - m_write_idx + m_read_idx - 1 : m_read_idx - m_write_idx - 1;
    if (write_space < buffer.size() && !blocking) return EAGAIN;

    uSize bytes_written = 0;
    while (bytes_written < buffer.size()) {
        // Total space we can write contiguously.
        auto segment_write_space =
            (m_write_idx >= m_read_idx) ? m_data.size() - m_write_idx : m_read_idx - m_write_idx - 1;
        if (m_read_idx == 0) segment_write_space -= 1;

        auto to_write = bek::min(segment_write_space, buffer.size() - bytes_written);
        EXPECTED_TRY(buffer.read_to(m_data.data() + m_write_idx, to_write, bytes_written));
        bytes_written += to_write;
        m_write_idx += to_write;
        if (m_write_idx == m_data.size()) m_write_idx = 0;
    }
    return bytes_written;
}
expected<uSize> Pipe::read(TransactionalBuffer& buffer, bool blocking) {
    uSize read_space;
    do {
        read_space = (m_write_idx >= m_read_idx) ? m_write_idx - m_read_idx : m_data.size() - m_read_idx + m_write_idx;
    } while (!read_space && blocking);

    if (!read_space) return EAGAIN;

    uSize bytes_read = 0;
    while (read_space && bytes_read < buffer.size()) {
        auto segment_read_space = (m_write_idx >= m_read_idx) ? m_write_idx - m_read_idx : m_data.size() - m_read_idx;
        auto to_read = bek::min(segment_read_space, buffer.size() - bytes_read);
        EXPECTED_TRY(buffer.write_from(m_data.data() + m_read_idx, to_read, bytes_read));
        bytes_read += to_read;
        m_read_idx += to_read;
        if (m_read_idx == m_data.size()) m_read_idx = 0;

        read_space = (m_write_idx >= m_read_idx) ? m_write_idx - m_read_idx : m_data.size() - m_read_idx + m_write_idx;
    }
    return bytes_read;
}
