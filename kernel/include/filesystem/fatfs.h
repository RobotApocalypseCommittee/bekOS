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

#ifndef BEKOS_FATFS_H
#define BEKOS_FATFS_H

#include "filesystem.h"
#include "fat.h"

namespace fs {

class FATFilesystem;
class FATDirectoryEntry;

class FATEntry: public Entry {
public:
    static EntryRef make_ref(LocatedFATEntry entry, bek::shared_ptr<FATDirectoryEntry> parent);
    FATEntry(bool is_directory, bek::string t_name, const EntryTimestamps& timestamps, uSize size,
             bek::shared_ptr<FATDirectoryEntry> parent, u32 root_cluster, FATEntryLocation entry_location,
             FATFilesystem& filesystem);
    ErrorCode flush() override;

    EntryRef parent() const override;
    expected<bool> rename(bek::str_view new_name) override;
    expected<bool> reparent(EntryRef new_parent, bek::optional<bek::str_view> new_name) override;

protected:
    expected<uSize> fat_read_data(TransactionalBuffer& buffer, uSize offset, uSize length);
    expected<uSize> fat_write_data(TransactionalBuffer& buffer, uSize offset, uSize length);
    expected<uSize> fat_resize(uSize new_size);

    FATFilesystem& m_filesystem;
    bek::shared_ptr<FATDirectoryEntry> m_parent;

    /// The cluster where the file/directory starts.
    unsigned m_root_cluster = 0;
    FATEntryKind m_kind;
    FATEntryLocation m_entry_location;
};

// Can be the root node too.
class FATDirectoryEntry: public FATEntry {
public:
    FATDirectoryEntry(const bek::string& name, const EntryTimestamps& timestamps, uSize size,
                      const bek::shared_ptr<FATDirectoryEntry>& parent, u32 root_cluster,
                      const FATEntryLocation& entry_location, FATFilesystem& filesystem);
    ErrorCode rename_child(FATEntry& child, bek::string new_name) {
        // TODO: Implement
        return ENOTSUP;
    }
    expected<EntryRef> lookup(bek::str_view name) override;
    expected<bek::vector<EntryRef>> all_children() override;

private:
    friend class FATFilesystem;
};

class FATFileEntry: public FATEntry {
public:
    FATFileEntry(const bek::string& name, const EntryTimestamps& timestamps, uSize size,
                 const bek::shared_ptr<FATDirectoryEntry>& parent, u32 root_cluster,
                 const FATEntryLocation& entry_location, FATFilesystem& filesystem);
    expected<uSize> write_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) override;
    expected<uSize> read_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) override;
    expected<uSize> resize(uSize new_size) override;

public:
public:
    using FATEntry::FATEntry;
};

class FATFilesystem final : public Filesystem {
public:
    static expected<bek::own_ptr<FATFilesystem>> try_create_from(blk::BlockDevice& device);
    FATFilesystem(blk::BlockDevice& partition, FATInfo& info);

    EntryRef get_root() override;

    FileAllocationTable& get_fat() { return fat; }

private:
    FileAllocationTable fat;
    bek::shared_ptr<FATDirectoryEntry> root_directory;
};

}
#endif //BEKOS_FATFS_H
