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

#ifndef BEKOS_FILESYSTEM_H
#define BEKOS_FILESYSTEM_H

// Info about file/directory without accessing data

#include "hardtypes.h"

#include "library/vector.h"
#include "library/acquirable_ref.h"

class FilesystemEntry;
typedef AcquirableRef<FilesystemEntry> FilesystemEntryRef;

class FilesystemEntry {
public:
    // Owns string
    char* m_name;
    bool open = false;
    u64 last_modified;
    u64 created;
    unsigned size = 0;
    FilesystemEntryRef parent;
    enum FilesystemEntryType {
        DIRECTORY,
        FILE
    } type;

    void setParent(FilesystemEntryRef newParent);

    FilesystemEntry(const FilesystemEntry &) = delete;

    FilesystemEntry() = default;

    FilesystemEntry &operator=(const FilesystemEntry &) = delete;

    virtual ~FilesystemEntry();

    void acquire();
    void release();
    unsigned get_ref_count();

    void mark_dirty();

    virtual vector<FilesystemEntryRef> enumerate() = 0;
    virtual FilesystemEntryRef lookup(const char* name) = 0;

    u64 get_hash();

protected:
    virtual void commit_changes() = 0;

private:
    bool _dirty{false};
    unsigned ref_count{0};
    u64 m_hash{0};
};

u64 entry_hash(u64 previous, const char* name);

// An open file
struct File {
public:
    virtual bool read(void* buf, size_t length, size_t offset) = 0;
    virtual bool write(void* buf, size_t length, size_t offset) = 0;
    virtual bool resize(size_t new_length) = 0;

    virtual bool close() = 0;

    virtual ~File() = default;
};

class EntryHashtable;

class Filesystem {
public:
    explicit Filesystem(EntryHashtable* entryCache);

    virtual FilesystemEntryRef getInfo(char* path) = 0;
    virtual FilesystemEntryRef getRootInfo() = 0;

    virtual File* open(FilesystemEntryRef entry) = 0;


    EntryHashtable* entryCache;
};

#endif //BEKOS_FILESYSTEM_H
