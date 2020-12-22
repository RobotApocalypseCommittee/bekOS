/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "filesystem/fatfs.h"

#include "filesystem/entrycache.h"
#include "library/optional.h"

using bek::readLE;
using namespace fs;

bek::optional<FATInfo> fromBootSector(BlockDevice &device) {
    assert(device.block_size() >= 512);
    u8 buffer[512];
    device.readBlock(0, &buffer[0], 0, 512);

    if (readLE<u32>(&buffer[0x1FE]) != 0xAA55) return {};
    if (buffer[0x42] != 0x28 && buffer[0x42] != 0x29) return {};
    auto sector_size = readLE<u16>(&buffer[0x0B]);
    auto sectors_per_cluster = buffer[0x0D];
    auto reserved_sectors = readLE<u16>(&buffer[0x0E]);
    auto fat_count = buffer[0x10];
    auto sectors_per_fat = readLE<u32>(&buffer[0x24]);
    auto root_begin_cluster = readLE<u32>(&buffer[0x2C]);
    auto fat_begin_sector = reserved_sectors;
    auto clusters_begin_sector = reserved_sectors + fat_count * sectors_per_fat;
    return FATInfo{sector_size, sectors_per_cluster, fat_begin_sector, sectors_per_fat, clusters_begin_sector, root_begin_cluster};
}

EntryRef FATFilesystem::getRootInfo() {
    if (root_directory.empty()) {
        auto entry = fat.getRootEntry();
        root_directory = bek::AcquirableRef(new fs::FATDirectoryEntry(entry, *this, {}));
        entryCache.insert(root_directory);
    }
    return root_directory;
}


FATEntry::FATEntry(const RawFATEntry &t_entry, FATFilesystem &t_filesystem, EntryRef t_parent)
        : filesystem(t_filesystem), m_root_cluster(t_entry.start_cluster), m_source_cluster(t_entry.source_cluster),
          m_source_index(t_entry.source_cluster_entry) {
    size = t_entry.size;
    parent = bek::move(t_parent);
}

void FATEntry::commit_changes() {
    // TODO
}

File *FATFilesystem::open(EntryRef entry) {
    entry->open = true;
    auto x = new FATFile(entry);
    return x;
}

FileAllocationTable &FATFilesystem::getFAT() {
    return fat;
}

FATFilesystem::FATFilesystem(BlockDevice &partition, EntryHashtable &entryCache) : Filesystem(entryCache),
                                                                                   fat(fromBootSector(
                                                                                           partition).release(),
                                                                                       partition) {

}


bool FATFile::resize(uSize new_length) {
    if (new_length > fileEntry->size) {
        // Do nothing unless extending
        if (!fileEntry->filesystem.getFAT().extendFile(fileEntry->m_root_cluster, new_length)) {
            return false;
        }
    }
    fileEntry->size = new_length;
    fileEntry->mark_dirty();
    return true;
}

bool FATFile::read(void *buf, uSize length, uSize offset) {
    return fileEntry->filesystem.getFAT().readData(buf, fileEntry->m_root_cluster, offset, length);
}

bool FATFile::write(void *buf, uSize length, uSize offset) {
    return fileEntry->filesystem.getFAT().writeData(buf, fileEntry->m_root_cluster, offset, length);
}

bool FATFile::close() {
    // Seems dodgy?
    fileEntry = nullptr;
    return true;
}

Entry &FATFile::getEntry() {
    return *fileEntry;
}

FATFile::FATFile(bek::AcquirableRef<FATFileEntry> fileEntry) : fileEntry(bek::move(fileEntry)) {}

bek::vector<EntryRef> FATDirectoryEntry::enumerate() {
    bek::vector<EntryRef> result;
    for (auto &e: filesystem.getFAT().getEntries(m_root_cluster)) {
        if (e.type == ::RawFATEntry::DIRECTORY) {
            result.push_back(EntryRef{new FATDirectoryEntry(e, filesystem, this)});
        } else {
            result.push_back(EntryRef{new FATFileEntry(e, filesystem, this)});
        }
    }
    return result;
}

EntryRef FATDirectoryEntry::lookup(bek::string_view name) {
    for (auto &e: filesystem.getFAT().getEntries(m_root_cluster)) {
        if (strcmp(name.data(), e.name.data()) == 0) {
            // Found
            return EntryRef{
                    e.type == ::RawFATEntry::DIRECTORY ? static_cast<Entry *>(new FATDirectoryEntry(e, filesystem,
                                                                                                    this))
                                                       : static_cast<Entry *>(new FATFileEntry(e, filesystem, this))};
        }
    }
    return {};
}
