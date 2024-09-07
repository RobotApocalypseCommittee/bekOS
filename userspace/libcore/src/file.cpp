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

#include "core/file.h"

#include "core/syscall.h"

inline constexpr uSize BUFFER_SIZE = 1024;

core::BufferedFile core::BufferedFile::create(int fd, sc::OpenFlags flags) { return BufferedFile(fd, flags); }
core::BufferedFile::BufferedFile(int fd, sc::OpenFlags flags, core::BufferedFile::BufferingMode buffering_mode)
    : m_buffer(buffering_mode == NO_BUFFERING ? 0 : BUFFER_SIZE),
      write_base{0},
      write_pos{0},
      write_end{m_buffer.size()},
      read_pos{0},
      read_end{0},
      open_flags{flags},
      m_buffering_mode{buffering_mode},
      m_fd{fd} {}

core::expected<uSize> core::BufferedFile::read(void* buf, uSize len) {
    if (!(open_flags & sc::OpenFlags::Read)) return EBADF;
    auto* cbuf = static_cast<unsigned char*>(buf);
    auto length_left = len;

    auto read_from_buffer = [&]() {
        auto to_read = bek::min(length_left, read_end - read_pos);
        bek::memcopy(cbuf, m_buffer.data() + read_pos, to_read);
        length_left -= to_read;
        cbuf += to_read;
        read_pos += to_read;
    };

    // Serve bytes from buffer if possible.
    if (read_end - read_pos) {
        read_from_buffer();
    }

    if (length_left == 0) {
        return len;
    }

    // If still not complete, fetch some more.
    if (length_left < m_buffer.size()) {
        // Fill buffer - first need to flush.
        EXPECTED_TRY(flush_writes(true));

        while (length_left) {
            auto length_read =
                EXPECTED_TRY(core::syscall::read(m_fd, sc::INVALID_OFFSET_VAL, m_buffer.data(), m_buffer.size()));
            if (length_read == 0) {
                // EOF
                return len - length_left;
            }

            read_pos = 0;
            read_end = length_read;
            read_from_buffer();
        }

        return len;
    } else {
        while (length_left) {
            // Large read - we don't bother dealing with buffer.
            auto length_read = EXPECTED_TRY(core::syscall::read(m_fd, sc::INVALID_OFFSET_VAL, cbuf, length_left));

            if (length_read == 0) {
                // EOF
                return len - length_left;
            }
            cbuf += length_read;
            length_left -= length_read;
        }
        return len;
    }
}
core::expected<uSize> core::BufferedFile::flush_writes(bool preparing_for_read) {
    while (write_pos - write_base) {
        // There are unflushed writes.
        auto written = EXPECTED_TRY(
            core::syscall::write(m_fd, sc::INVALID_OFFSET_VAL, m_buffer.data() + write_base, write_pos - write_base));
        write_base += written;
        // EOF spells disaster for us.
        if (written == 0) return EIO;
    }

    if (preparing_for_read) {
        // We don't try to allocate a write buffer.
        write_base = write_pos = 0;
        write_end = 0;
    } else if (read_pos > 0) {
        write_base = write_pos = 0;
        write_end = read_pos;
    } else {
        write_base = write_pos = read_end;
        write_end = m_buffer.size();
    }
    return 0ull;
}
core::expected<uSize> core::BufferedFile::write(const void* buf, uSize len) {
    if (!(open_flags & sc::OpenFlags::Write)) return EBADF;

    auto* cbuf = static_cast<const char*>(buf);
    // What to do with read_buffer? Answer - don't disturb it.
    uSize written = 0;

    auto find_next_line_end = [](const char* buf, uSize length) -> const char* {
        auto end = buf + length;
        for (auto* it = buf; it < end; it++) {
            if (*it == '\n') return it + 1;
        }
        return nullptr;
    };

    // Can be nullptr. Next point at which a flush is mandatory.
    auto next_flush_point = (m_buffering_mode == LINE_BUFFERED) ? find_next_line_end(cbuf, len) : nullptr;

    if (write_end - write_pos) {
        // Space in buffer - use it!
        auto to_copy = bek::min(len, write_end - write_pos);
        bek::memcopy(m_buffer.data() + write_pos, cbuf, to_copy);
        write_pos += to_copy;

        len -= to_copy;
        written += to_copy;
        cbuf += to_copy;
    }

    // A flush is required if we moved past a flush point OR if we filled the buffer.
    if (write_end == write_pos || (next_flush_point && next_flush_point <= cbuf)) {
        EXPECTED_TRY(flush_writes(false));
        // We update next_flush_point if it is now in the past.
        if (next_flush_point && next_flush_point <= cbuf) {
            next_flush_point = find_next_line_end(cbuf, len);
        }
    }

    if (len == 0) return written;

    // At this point, next_flush_point is still correct, the write_buffer will be empty and ready
    // for stuff.

    // If the remaining length fits in the (new) buffer AND we don't have to flush anyway, use
    // buffer.
    if (len < (write_end - write_pos) && !next_flush_point) {
        bek::memcopy(m_buffer.data() + write_pos, cbuf, len);
        write_pos += len;
        written += len;
        return written;
    } else {
        // Skip buffer.
        while (len) {
            auto did_write = EXPECTED_TRY(core::syscall::write(m_fd, sc::INVALID_OFFSET_VAL, cbuf, len));
            cbuf += did_write;
            written += did_write;
            len -= did_write;
            // EOF
            if (did_write == 0) {
                // EOF
                break;
            }
        }
        return written;
    }
}
core::expected<uSize> core::BufferedFile::seek(sc::SeekLocation mode, iSize offset) {
    read_pos = read_end = 0;
    EXPECTED_TRY(flush_writes(false));
    return core::syscall::seek(m_fd, mode, offset).map_value([](auto v) { return static_cast<uSize>(v); });
}
core::expected<uSize> core::BufferedFile::flush() {
    EXPECTED_TRY(flush_writes(false));
    return 0ull;
}
