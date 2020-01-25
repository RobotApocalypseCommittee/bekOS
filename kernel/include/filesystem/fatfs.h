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

class FATFilesystem;

class FATFilesystemEntry: public FilesystemEntry {
public:
    explicit FATFilesystemEntry(const FATEntry&);

    void setFATValues(u32 root_cluster, u32 source_cluster, u32 source_index);
    void setFilesystem(FATFilesystem* pFilesystem, FileAllocationTable* pTable);
protected:
    FATFilesystem* filesystem = nullptr;
    FileAllocationTable* table = nullptr;
    unsigned m_root_cluster = 0;
    unsigned m_source_cluster = 0;
    unsigned m_source_index = 0;

friend class FATFile;
};

class FATFilesystemDirectory: public FATFilesystemEntry {
public:
    using FATFilesystemEntry::FATFilesystemEntry;
    vector<FilesystemEntryRef> enumerate() override;

    FilesystemEntryRef lookup(const char* name) override;
private:
    bool getNextEntry(FATEntry* nextEntry, u32& cluster_n, u32& entry_n);
};
class FATFilesystemFile: public FATFilesystemEntry {
public:
    using FATFilesystemEntry::FATFilesystemEntry;
    vector<FilesystemEntryRef> enumerate() override;

    FilesystemEntryRef lookup(const char* name) override;

};

class FATFile: public File {
public:
    explicit FATFile(AcquirableRef<FATFilesystemFile> fileEntry);

    bool read(void* buf, size_t length, size_t offset) override;

    bool write(void* buf, size_t length, size_t offset) override;

    bool close() override;

private:
    AcquirableRef<FATFilesystemFile> fileEntry;
};

class FATFilesystem: public Filesystem {
public:
    FilesystemEntryRef getInfo(char* path) override;

    FilesystemEntryRef getRootInfo() override;

    File* open(FilesystemEntryRef entry) override;

    explicit FATFilesystem(partition* partition, EntryHashtable* entryCache);

private:
    FileAllocationTable fat;
    AcquirableRef<FATFilesystemDirectory> root_directory;
};

#endif //BEKOS_FATFS_H
