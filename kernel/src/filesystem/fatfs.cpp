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

using namespace bek;



bek::optional<FATInfo> fromBootSector(BlockDevice& device) {
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

bek::vector<FilesystemEntryRef> FATFilesystemFile::enumerate() {
    assert(0);
    return bek::vector<FilesystemEntryRef>();
}

FilesystemEntryRef FATFilesystemFile::lookup(const char* name) {
    (void) name;
    assert(0);
    return FilesystemEntryRef();
}

FilesystemEntryRef FATFilesystem::getInfo(char* path) {
    (void) path;
    // TODO: DO
    return FilesystemEntryRef();
}

FilesystemEntryRef FATFilesystem::getRootInfo() {
    if (root_directory.empty()) {
        auto entry = fat.getRootEntry();
        root_directory = bek::AcquirableRef<FATFilesystemDirectory>(new FATFilesystemDirectory(entry, *this));
        entryCache->insert(root_directory);
    }
    return root_directory;
}


File* FATFilesystem::open(FilesystemEntryRef entry) {
    auto x = new FATFile(entry);
    entry->open = true;
    return x;
}

FileAllocationTable &FATFilesystem::getFAT() {
    return fat;
}

FATFilesystem::FATFilesystem(BlockDevice &partition, EntryHashtable &entryCache): Filesystem(&entryCache), fat(fromBootSector(partition).release(), partition) {

}

bool FATFile::read(void* buf, size_t length, size_t offset) {
    if (offset + length > fileEntry->size) {
        return false;
    }
    return fileEntry->filesystem.getFAT().readData(buf, fileEntry->m_root_cluster, offset, length);
}

bool FATFile::write(void* buf, size_t length, size_t offset) {
    if (offset + length > fileEntry->size) {
        return false;
    }
    return fileEntry->filesystem.getFAT().writeData(buf, fileEntry->m_root_cluster, offset, length);
}

bool FATFile::close() {
    fileEntry->open = false;
    return true;
}

FATFile::FATFile(bek::AcquirableRef<FATFilesystemFile> fileEntry) : fileEntry(std::move(fileEntry)) {}

bool FATFile::resize(size_t new_length) {
    if (new_length > fileEntry->size) {
        // Do nothing unless extending
        if (!fileEntry->filesystem.getFAT().extendFile(fileEntry->m_root_cluster, new_length)){
            return false;
        }
    }
    fileEntry->size = new_length;
    fileEntry->mark_dirty();
    return true;
}

FilesystemEntry* FATFile::getFilesystemEntry() {
    return fileEntry.get();
}
