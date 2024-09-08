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

#ifndef BEKOS_CORE_FILE_H
#define BEKOS_CORE_FILE_H

#include "api/syscalls.h"
#include "bek/format_core.h"
#include "bek/vector.h"
#include "error.h"

namespace core {

class BufferedFile {
public:
    enum BufferingMode {
        NO_BUFFERING,
        LINE_BUFFERED,
        BLOCK_BUFFERED,
    };

    static BufferedFile create(int fd, sc::OpenFlags flags);

    expected<uSize> seek(sc::SeekLocation mode, iSize offset);
    expected<uSize> read(void* buf, uSize len);
    expected<uSize> write(const void* buf, uSize len);
    expected<uSize> flush();

    explicit BufferedFile(int fd, sc::OpenFlags flags, BufferingMode buffering_mode = BLOCK_BUFFERED);

private:
    core::expected<uSize> flush_writes(bool preparing_for_read);

    bek::vector<u8> m_buffer;
    uSize write_base;
    uSize write_pos;
    uSize write_end;
    uSize read_pos;
    uSize read_end;
    sc::OpenFlags open_flags;
    BufferingMode m_buffering_mode;
    int m_fd;
};

class BufferedFileOutputStream : public bek::OutputStream {
public:
    explicit BufferedFileOutputStream(core::BufferedFile& f) : m_f{f} {}
    void write(bek::str_view str) override { m_f.write(str.data(), str.size()); }
    void write(char c) override { m_f.write(&c, 1); }
    void reserve(uSize n) override { (void)n; }

private:
    core::BufferedFile& m_f;
};
}  // namespace core

#endif  // BEKOS_CORE_FILE_H
