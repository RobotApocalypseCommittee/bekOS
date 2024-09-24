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

#ifndef BEKOS_FILESYSTEM_H
#define BEKOS_FILESYSTEM_H

// Info about file/directory without accessing data

#include "bek/own_ptr.h"
#include "bek/str.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "filesystem/entry.h"
#include "library/hashtable.h"
#include "library/intrusive_shared_ptr.h"
#include "library/kernel_error.h"
#include "path.h"
#include "process/entity.h"

namespace fs {

struct FileHandle final : EntityHandle {
public:
    using EntityType = FileHandle;
    explicit FileHandle(EntryRef entry_ref) : m_entry(bek::move(entry_ref)) {}
    static constexpr Kind EntityKind = Kind::File;
    Kind kind() const override { return Kind::File; }

    Entry& entry() const { return *m_entry; }
    expected<uSize> read(u64 offset, TransactionalBuffer& buffer) override {
        uSize actual_offset = (offset == sc::INVALID_OFFSET_VAL) ? m_offset : offset;
        auto r = m_entry->read_bytes(buffer, actual_offset, buffer.size());
        if (r.has_value()) {
            m_offset = actual_offset + r.value();
        }
        return r;
    }

    expected<uSize> write(u64 offset, TransactionalBuffer& buffer) override {
        uSize actual_offset = (offset == sc::INVALID_OFFSET_VAL) ? m_offset : offset;
        auto r = m_entry->write_bytes(buffer, actual_offset, buffer.size());
        if (r.has_value()) {
            m_offset = actual_offset + r.value();
        }
        return r;
    }

    SupportedOperations get_supported_operations() const override {
        return (m_entry->is_directory()) ? None : (Read | Write | Seek);
    }
    expected<uSize> seek(sc::SeekLocation position, i64 offset) override {
        uSize start_point = 0;

        switch (position) {
            case sc::SeekLocation::Start:
                start_point = 0;
                break;
            case sc::SeekLocation::Current:
                start_point = m_offset;
                break;
            case sc::SeekLocation::End:
                start_point = m_entry->size();
                break;
        }
        // TODO: More rigorous checking?
        if (offset < 0 && uSize(-offset) > start_point) {
            return EINVAL;
        }

        if (offset > 0 && ((iSize)start_point + offset < 0 || start_point + offset > m_entry->size())) {
            return EINVAL;
        }

        m_offset = start_point + offset;
        return m_offset;
    }

private:
    EntryRef m_entry;
    uSize m_offset{};
};

class Filesystem {
public:
    virtual EntryRef get_root() = 0;
    virtual ~Filesystem() = default;
};

class FilesystemRegistry {
public:
    /// Initialises filesystem and registers as root the first filesystem so registered.
    static void register_filesystem(bek::string name, bek::own_ptr<Filesystem> fs);
    static ErrorCode try_mount_root();
    static FilesystemRegistry& the();

    expected<EntryRef> lookup_root(const path& the_path);

private:
    FilesystemRegistry(bek::string root_name, bek::own_ptr<Filesystem> root_fs);

    bek::hashtable<bek::string, bek::own_ptr<Filesystem>> m_filesystems;
    Filesystem& m_root_filesystem;
};

expected<EntryRef> fullPathLookup(EntryRef root, const path& s, EntryRef* out_parent);
expected<EntryRef> fullPathLookup(EntryRef root, bek::str_view path, EntryRef* out_parent);
bek::shared_ptr<fs::FileHandle> open_file(EntryRef entry);

}
#endif //BEKOS_FILESYSTEM_H
