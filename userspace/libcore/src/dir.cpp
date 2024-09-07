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

#include "core/dir.h"

#include "bek/allocations.h"
#include "core/syscall.h"

constexpr uSize DIRECTORY_STREAM_BUFFER_SIZE = 1024;
namespace core {

expected<DirectoryStream> DirectoryStream::create(int fd) {
    auto buf = mem::allocate(DIRECTORY_STREAM_BUFFER_SIZE, alignof(sc::FileListItem));
    // Don't bother with OOM error handling.
    VERIFY(buf.pointer);

    // Do first fetch
    auto new_offset = EXPECTED_TRY(core::syscall::get_directory_entries(fd, 0, buf.pointer, buf.size));

    return DirectoryStream{fd, buf.pointer, buf.size, static_cast<uSize>(new_offset)};
}
const sc::FileListItem* DirectoryStream::advance() {
    // Look at current item.
    auto& cur_item = m_buffer.get_at<sc::FileListItem>(m_offset_in_buffer);
    if (cur_item.next_offset == 0) {
        // End of Directory
        return nullptr;
    } else if ((m_offset_in_buffer + cur_item.next_offset) >= m_buffer.size()) {
        // Need next buffer.
        auto get_result =
            core::syscall::get_directory_entries(m_fd, m_next_os_offset, m_buffer.data(), m_buffer.size());
        if (get_result) {
            m_next_os_offset = get_result.value();
            m_offset_in_buffer = 0;
            return &m_buffer.get_at<sc::FileListItem>(m_offset_in_buffer);
        } else {
            m_error = get_result.error();
            return nullptr;
        }
    } else {
        // Just get next thing.
        m_offset_in_buffer += cur_item.next_offset;
        return &m_buffer.get_at<sc::FileListItem>(m_offset_in_buffer);
    }
}
DirectoryStream::Iterator DirectoryStream::begin() {
    VERIFY(m_offset_in_buffer == 0);
    return DirectoryStream::Iterator{DirectoryEntry{m_buffer.get_at<sc::FileListItem>(0)}, *this};
}
DirectoryStream::Iterator DirectoryStream::end() { return DirectoryStream::Iterator{bek::nullopt, *this}; }

bool operator==(const DirectoryStream::Iterator& a, const DirectoryStream::Iterator& b) {
    return (&a.m_directory_stream == &b.m_directory_stream) && (a.m_entry == b.m_entry);
}

DirectoryEntry::DirectoryEntry(const sc::FileListItem& item)
    : name{item.name_view()}, kind{item.kind}, size{item.size} {}
}  // namespace core