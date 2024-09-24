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
#include "filesystem/fatfs.h"

#include "bek/optional.h"

using bek::readLE;
using namespace fs;

bek::optional<FATInfo> fromBootSector(blk::BlockDevice& device) {
    u8 buffer[512];
    blk::blocking_read(device, 0, {(char*)&buffer[0], 512});

    if (readLE<u16>(&buffer[0x1FE]) != 0xAA55) return {};

    auto sector_size = readLE<u16>(&buffer[11]);
    // Sector size must be power of two in [512, 4096]
    if ((sector_size & (sector_size - 1)) != 0 || sector_size < 512 || sector_size > 4096) {
        return {};
    }

    auto sectors_per_cluster = buffer[13];
    auto reserved_sectors = readLE<u16>(&buffer[14]);
    auto fat_count = buffer[0x10];

    auto root_entries_16 = readLE<u16>(&buffer[17]);
    auto total_sectors_16 = readLE<u16>(&buffer[19]);
    // Skip media label
    auto sectors_per_fat_16 = readLE<u16>(&buffer[22]);
    // Skip Geometry
    auto total_sectors_32 = readLE<u32>(&buffer[32]);

    // FAT 32 extra data
    auto sectors_per_fat_32 = readLE<u32>(&buffer[36]);
    auto root_begin_cluster_32 = readLE<u32>(&buffer[44]);

    // Combine some mixed values
    u32 total_sectors = (total_sectors_16 == 0) ? total_sectors_32 : total_sectors_16;
    u32 sectors_per_fat = (sectors_per_fat_16 == 0) ? sectors_per_fat_32 : sectors_per_fat_16;

    // Only valid for FAT16:
    u32 root_dir_sectors = bek::ceil_div((u32)root_entries_16 * 32, (u32)sector_size);

    // FAT 16 excludes root dir
    u32 data_begin_sector = reserved_sectors + fat_count * sectors_per_fat + root_dir_sectors;

    u32 data_sectors = total_sectors - data_begin_sector;
    u32 total_clusters = total_sectors / sectors_per_cluster;

    FatType fat_type;
    if (sector_size == 0) {
        fat_type = FatType::ExFAT;
    } else if (total_clusters < 4085) {
        fat_type = FatType::FAT12;
    } else if (total_clusters < 65525) {
        fat_type = FatType::FAT16;
    } else {
        fat_type = FatType::FAT32;
    }

    if (fat_type == FatType::ExFAT || fat_type == FatType::FAT12) return {};

    FATInfo::RootAttrs root_attrs{};
    if (fat_type == FatType::FAT32) {
        root_attrs.root_dir_cluster = root_begin_cluster_32;
    } else {
        root_attrs.root_dir_attrs_16 = {static_cast<u16>(reserved_sectors + fat_count * sectors_per_fat),
                                        root_entries_16};
    }
    return FATInfo{fat_type,        sector_size, sectors_per_cluster, reserved_sectors,
                   sectors_per_fat, root_attrs,  data_begin_sector};
}

EntryRef FATFilesystem::get_root() { return root_directory; }
FATFilesystem::FATFilesystem(blk::BlockDevice& partition, FATInfo& info) : fat(info, partition) {
    auto* root = new FATDirectoryEntry(bek::string("root"), EntryTimestamps{}, 0, {}, 0, FATEntryLocation{0, 0}, *this);
    VERIFY(root);
    root_directory = bek::adopt_shared(root);
}
expected<bek::own_ptr<FATFilesystem>> FATFilesystem::try_create_from(blk::BlockDevice& device) {
    auto info_o = fromBootSector(device);
    if (!info_o) return EINVAL;
    auto filesystem = new FATFilesystem(device, *info_o);
    if (!filesystem) return ENOMEM;
    return bek::own_ptr{filesystem};
}

EntryRef FATEntry::parent() const { return m_parent; }

expected<bool> FATEntry::rename(bek::str_view new_name) {
    if (m_kind == FATEntryKind::Root) {
        m_name = bek::string{new_name};
        return true;
    } else {
        ASSERT(m_parent);
        auto code = m_parent->rename_child(*this, bek::string{new_name});
        if (code == ESUCCESS) {
            return true;
        } else {
            return code;
        }
    }
}
ErrorCode FATEntry::flush() {
    // Flush adjusts the following attributes: timestamps, size, root cluster.
    if (m_kind == FATEntryKind::Root) {
        return ESUCCESS;
    }
    // Get current entry data
    auto raw_entry = EXPECTED_TRY(m_filesystem.get_fat().get_entry(m_entry_location));
    raw_entry.size = size();
    if (m_timestamps.accessed) {
        raw_entry.accessed_timestamp = *m_timestamps.accessed;
    }
    if (m_timestamps.modified) {
        raw_entry.modified_timestamp = *m_timestamps.modified;
    }
    if (m_timestamps.created) {
        raw_entry.creation_timestamp = *m_timestamps.created;
    }
    m_entry_location = EXPECTED_TRY(m_filesystem.get_fat().update_entry(m_entry_location, raw_entry));
    return ESUCCESS;
}
expected<uSize> FATEntry::fat_read_data(TransactionalBuffer& buffer, uSize offset, uSize length) {
    if (m_filesystem.get_fat().doDataInterchange(buffer, m_root_cluster, offset, length, false)) {
        return length;
    } else {
        return EIO;
    }
}
expected<uSize> FATEntry::fat_write_data(TransactionalBuffer& buffer, uSize offset, uSize length) {
    if (m_filesystem.get_fat().doDataInterchange(buffer, m_root_cluster, offset, length, true)) {
        return length;
    } else {
        return EIO;
    }
}
expected<uSize> FATEntry::fat_resize(uSize new_size) {
    if (new_size > size()) {
        if (!m_filesystem.get_fat().extendFile(m_root_cluster, new_size)) {
            return EIO;
        }
    }
    m_size = new_size;
    m_dirty = true;
    return new_size;
}
EntryRef FATEntry::make_ref(LocatedFATEntry entry, bek::shared_ptr<FATDirectoryEntry> parent) {
    auto& e = entry.entry;
    FATEntry* new_entry;
    EntryTimestamps timestamps{e.creation_timestamp, e.modified_timestamp, e.accessed_timestamp};
    VERIFY(parent);
    auto& filesystem = parent->m_filesystem;
    if (entry.entry.is_directory()) {
        new_entry = new FATDirectoryEntry(e.name, timestamps, e.size, bek::move(parent), e.data_cluster, entry.location,
                                          filesystem);
    } else {
        new_entry =
            new FATFileEntry(e.name, timestamps, e.size, bek::move(parent), e.data_cluster, entry.location, filesystem);
    }
    return {EntryRef::AdoptTag{}, new_entry};
}
expected<bool> FATEntry::reparent(EntryRef new_parent, bek::optional<bek::str_view> new_name) { return ENOTSUP; }
FATEntry::FATEntry(bool is_directory, bek::string t_name, const EntryTimestamps& timestamps, uSize size,
                   bek::shared_ptr<FATDirectoryEntry> parent, u32 root_cluster, FATEntryLocation entry_location,
                   FATFilesystem& filesystem)
    : Entry(is_directory, bek::move(t_name), timestamps, size),
      m_filesystem{filesystem},
      m_root_cluster{root_cluster},
      m_kind{parent ? (parent->parent() ? FATEntryKind::Normal : FATEntryKind::RootMember) : FATEntryKind::Root},
      m_entry_location{entry_location} {}

expected<uSize> FATFileEntry::write_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) {
    return fat_write_data(buffer, offset, length);
}
expected<uSize> FATFileEntry::read_bytes(TransactionalBuffer& buffer, uSize offset, uSize length) {
    return fat_read_data(buffer, offset, length);
}
expected<uSize> FATFileEntry::resize(uSize new_size) { return fat_resize(new_size); }
FATFileEntry::FATFileEntry(const bek::string& name, const EntryTimestamps& timestamps, uSize size,
                           const bek::shared_ptr<FATDirectoryEntry>& parent, u32 root_cluster,
                           const FATEntryLocation& entry_location, FATFilesystem& filesystem)
    : FATEntry(false, name, timestamps, size, parent, root_cluster, entry_location, filesystem) {}

expected<EntryRef> FATDirectoryEntry::lookup(bek::str_view name) {
    // FIXME: Use an iterator or caching or something!
    if (m_kind == FATEntryKind::Root) {
        for (auto& e : m_filesystem.get_fat().get_root_entries()) {
            if (e.entry.name.view() == name) {
                return make_ref(bek::move(e), (this));
            }
        }
    } else {
        for (auto& e : m_filesystem.get_fat().get_entries(m_root_cluster)) {
            if (e.entry.name.view() == name) {
                return make_ref(bek::move(e), (this));
            }
        }
    }
    return ENOENT;
}

expected<bek::vector<EntryRef>> FATDirectoryEntry::all_children() {
    bek::vector<EntryRef> res;
    bek::shared_ptr<FATDirectoryEntry> this_ent{this};
    auto entries = m_kind == FATEntryKind::Root ? m_filesystem.get_fat().get_root_entries()
                                                : m_filesystem.get_fat().get_entries(m_root_cluster);
    for (auto& e : entries) {
        res.push_back(make_ref(bek::move(e), this_ent));
    }
    return res;
}

FATDirectoryEntry::FATDirectoryEntry(const bek::string& name, const EntryTimestamps& timestamps, uSize size,
                                     const bek::shared_ptr<FATDirectoryEntry>& parent, u32 root_cluster,
                                     const FATEntryLocation& entry_location, FATFilesystem& filesystem)
    : FATEntry(true, name, timestamps, size, parent, root_cluster, entry_location, filesystem) {}
