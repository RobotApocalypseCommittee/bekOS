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

#ifndef BEKOS_SECTORCACHE_H
#define BEKOS_SECTORCACHE_H

#include <hardtypes.h>
#include "filesystem.h"


struct CacheEntry {
    u64 entryID;
    u64 size;
    void* data;
};

struct InternalCacheEntry;

// Stores CacheEntries in linked list and BST
class SectorCache {
public:
    CacheEntry* lookup(u64 id);

    /// Adds an entry
    /// \note Caller must ensure entry does not already exist in cache.
    /// \param entry Filled with data
    void addEntry(CacheEntry entry);

private:

    void insertionRepair(InternalCacheEntry* node);

    u64 entry_count;
    u64 desired_count;
    InternalCacheEntry* first_entry;
    InternalCacheEntry* root_entry;

};

#endif //BEKOS_SECTORCACHE_H
