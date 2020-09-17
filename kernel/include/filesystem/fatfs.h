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

#ifndef BEKOS_FATFS_H
#define BEKOS_FATFS_H

#include "filesystem.h"
#include "fat.h"

namespace fs {

class FATFilesystem;

class FATEntry: public Entry {
public:
    explicit FATEntry(const RawFATEntry &t_entry, FATFilesystem &t_filesystem, EntryRef t_parent);

protected:
    void commit_changes() override;

    FATFilesystem& filesystem;
    unsigned m_root_cluster = 0;
    unsigned m_source_cluster = 0;
    unsigned m_source_index = 0;

friend class FATFile;
};

class FATDirectoryEntry: public FATEntry {
public:
    using FATEntry::FATEntry;

    bek::vector<EntryRef> enumerate() override;

    EntryRef lookup(bek::string_view name) override;
};
class FATFileEntry: public FATEntry {
public:
    using FATEntry::FATEntry;
};

class FATFile: public File {
public:
    explicit FATFile(bek::AcquirableRef<FATFileEntry> fileEntry);

    bool read(void* buf, size_t length, size_t offset) override;

    bool write(void* buf, size_t length, size_t offset) override;

    bool close() override;

    bool resize(size_t new_length) override;

    Entry &getEntry() override;

private:
    bek::AcquirableRef<FATFileEntry> fileEntry;
};


class FATFilesystem: public Filesystem {
public:
    explicit FATFilesystem(BlockDevice& partition, EntryHashtable& entryCache);

    EntryRef getRootInfo() override;

    File *open(EntryRef entry) override;

    FileAllocationTable &getFAT();

private:
    FileAllocationTable fat;
    bek::AcquirableRef<FATDirectoryEntry> root_directory;
};


}
#endif //BEKOS_FATFS_H
