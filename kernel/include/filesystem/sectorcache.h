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
#include <library/acquirable_ref.h>

struct CacheEntry {
    u64 address;
    void* data;
    unsigned int ref_count;
    bool dirty;
    bool loaded;

    void acquire();
    void release();
};

using CacheEntryRef = bek::AcquirableRef<CacheEntry>;

struct CacheNode;
struct InternalCacheEntry;

// Stores CacheEntries in linked list and search tree
class BlockCache {
public:
    CacheEntryRef get(u64 id);

    void removeEntry(u64 id);

private:
    CacheNode* splitNode(u64 id, CacheNode* current, CacheNode* parent);
    InternalCacheEntry* createEntry(u64 id);
    u64 entry_count;
    u64 desired_count;
    InternalCacheEntry* first_entry;
    CacheNode* root_entry;
    bek::spinlock lock;
    u64 block_size;

};

#endif //BEKOS_SECTORCACHE_H
