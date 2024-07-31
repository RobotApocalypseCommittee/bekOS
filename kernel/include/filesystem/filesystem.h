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

#include "bek/str.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "filesystem/entry.h"
#include "library/hashtable.h"
#include "library/intrusive_shared_ptr.h"
#include "library/kernel_error.h"
#include "library/own_ptr.h"
#include "path.h"
#include "process/entity.h"

namespace fs {
class EntryHashtable;
class Entry;

using EntryRef = bek::AcquirableRef<Entry>;

enum class EntryType {
    DIRECTORY,
    FILE
};

class Entry {
public:
    bek::string name;
    u64 last_modified;
    u64 created;
    EntryRef parent;
    unsigned size = 0;
    bool open = false;
    EntryType type;

    void setParent(EntryRef newParent);

    Entry(const Entry &) = delete;

    Entry() = default;

    Entry &operator=(const Entry &) = delete;

    virtual ~Entry();

    void acquire();

    void release();

    unsigned get_ref_count() const;

    void mark_dirty();

    u64 get_hash();

    virtual bek::vector<EntryRef> enumerate();

    virtual EntryRef lookup(bek::string_view name);

protected:
    virtual void commit_changes() = 0;

private:
    bool _dirty{false};
    unsigned ref_count{0};
    u64 m_hash{0};
};


u64 entry_hash(u64 previous, const char *name);

// An open file
struct File {
public:
    virtual bool read(void *buf, uSize length, uSize offset) = 0;

    virtual bool write(void *buf, uSize length, uSize offset) = 0;

    virtual bool resize(uSize new_length) = 0;

    virtual bool close() = 0;

    virtual ~File() = default;

    virtual Entry& getEntry() = 0;
};

class Filesystem {
public:
    explicit Filesystem(EntryHashtable &entryCache);

    virtual EntryRef getRootInfo() = 0;

    virtual File *open(EntryRef entry) = 0;

protected:
    EntryHashtable &entryCache;
};

EntryRef fullPathLookup(EntryRef root, bek::string_view s);
}
#endif //BEKOS_FILESYSTEM_H
