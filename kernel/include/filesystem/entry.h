// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
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

#ifndef BEKOS_ENTRY_H
#define BEKOS_ENTRY_H

#include "bek/optional.h"
#include "bek/own_ptr.h"
#include "bek/str.h"
#include "bek/vector.h"
#include <bek/intrusive_shared_ptr.h>
#include "library/kernel_error.h"
#include "library/transactional_buffer.h"

namespace fs {

class Entry;
using EntryRef = bek::shared_ptr<Entry>;

struct EntryTimestamps {
    bek::optional<u64> created;
    bek::optional<u64> modified;
    bek::optional<u64> accessed;
};

class Entry : public bek::RefCounted<Entry> {
public:
    // Disable copying and moving
    Entry(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry(bool is_directory, bek::string name, EntryTimestamps timestamps, uSize size);
    Entry& operator==(const Entry&) = delete;
    Entry& operator==(Entry&&) = delete;

    // Getters
    bek::str_view name() const { return m_name.view(); }
    EntryTimestamps timestamps() const { return m_timestamps; }
    virtual EntryRef parent() const = 0;
    uSize size() const { return m_size; }

    bool is_unique() const { return m_is_unique; }
    bool is_directory() const { return m_is_directory; }

    // Setters
    virtual expected<bool> rename(bek::str_view new_name) = 0;
    virtual expected<bool> reparent(EntryRef new_parent, bek::optional<bek::str_view> new_name) = 0;
    void set_timestamps(const EntryTimestamps& timestamps);

    virtual ErrorCode flush() = 0;

    // Children - only if directory
    virtual expected<EntryRef> lookup(bek::str_view name) {
        (void)name;
        return ENOTSUP;
    }
    virtual expected<bek::vector<EntryRef>> all_children() { return ENOTSUP; }

    virtual expected<EntryRef> add_child(bek::str_view name, bool is_directory) {
        (void)name;
        (void)is_directory;
        return ENOTSUP;
    };
    virtual ErrorCode remove_child(bek::str_view name) {
        (void)name;
        return ENOTSUP;
    }

    // Binary Contents - Only if File.
    virtual ErrorCode prepare_for_access() { return ENOTSUP; }
    virtual expected<uSize> write_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) { return ENOTSUP; }
    virtual expected<uSize> read_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) { return ENOTSUP; }
    virtual expected<uSize> resize(uSize new_size) { return ENOTSUP; }

    virtual ~Entry();

    u64 get_hash();

protected:
    u64 m_hash{0};
    bek::string m_name;
    EntryTimestamps m_timestamps;
    uSize m_size;
    bool m_dirty{false};
    bool m_is_unique{false};
    bool m_is_directory;
};

}  // namespace fs

#endif  // BEKOS_ENTRY_H
