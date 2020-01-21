/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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

#include "filesystem/filesystem.h"
#include "printf.h"

void FilesystemEntry::acquire() {
    // TODO: Safety
    ref_count++;
    printf("Acquired: %s, %d\n", m_name, ref_count);
}

void FilesystemEntry::release() {
    // TODO: Safety
    ref_count--;
    printf("Released: %s, %d\n", m_name, ref_count);
}

void FilesystemEntry::setParent(FilesystemEntryRef newParent) {
    parent = newParent;
}

u64 FilesystemEntry::get_hash() {
    if (m_hash == 0) {
        m_hash = entry_hash(parent.get() != nullptr ? parent->get_hash() : 0, m_name);
    }
    return m_hash;
}

unsigned FilesystemEntry::get_ref_count() {
    return ref_count;
}

FilesystemEntry::~FilesystemEntry() {
    printf("Deleting %s\n", m_name);
    delete []m_name;
}


u64 entry_hash(u64 previous, const char* name) {
    u64 hash;
    if (previous == 0) {
        hash = 5381;
    } else {
        hash = previous;
    }
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5u) + hash) + c;
    }
    return hash;
}

Filesystem::Filesystem(EntryHashtable* entryCache) : entryCache(entryCache) {}
