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

#ifndef BEKOS_CORE_DIR_H
#define BEKOS_CORE_DIR_H

#include "api/syscalls.h"
#include "bek/buffer.h"
#include "bek/optional.h"
#include "bek/str.h"
#include "error.h"

namespace core {

struct DirectoryEntry {
    bek::string name;
    sc::FileKind kind;
    uSize size;

    bool operator==(const DirectoryEntry&) const = default;
    explicit DirectoryEntry(const sc::FileListItem& item);
};

class DirectoryStream {
public:
    class Iterator {
    public:
        Iterator& operator++() {
            if (auto* item = m_directory_stream.advance(); item) {
                m_entry = DirectoryEntry{*item};
            } else {
                m_entry = bek::nullopt;
            }
            return *this;
        }

        [[gnu::warning("Advancing a returned iterator may not result in a sensible result.")]] Iterator operator++(
            int) {
            Iterator x = *this;
            ++(*this);
            return x;
        }

        DirectoryEntry& operator*() { return *m_entry; }

        DirectoryEntry* operator->() { return &*m_entry; }

    private:
        Iterator(bek::optional<DirectoryEntry> entry, DirectoryStream& directory_stream)
            : m_entry{bek::move(entry)}, m_directory_stream{directory_stream} {}
        /// If nullopt, then represents eof.
        bek::optional<DirectoryEntry> m_entry;
        DirectoryStream& m_directory_stream;
        friend class DirectoryStream;
        friend bool operator==(const Iterator&, const Iterator&);
    };

    static expected<DirectoryStream> create(int fd);
    Iterator begin();
    Iterator end();
    bool is_errored() const { return m_error != ESUCCESS; }
    ErrorCode error() const { return m_error; }

private:
    const sc::FileListItem* advance();
    DirectoryStream(int fd, void* buffer, uSize buffer_size, uSize next_offset)
        : m_fd{fd}, m_buffer{static_cast<char*>(buffer), buffer_size}, m_next_os_offset{next_offset} {}
    int m_fd;
    bek::mut_buffer m_buffer;
    uSize m_offset_in_buffer{};
    uSize m_next_os_offset{};
    ErrorCode m_error{ESUCCESS};
};

}  // namespace core

#endif  // BEKOS_CORE_DIR_H
