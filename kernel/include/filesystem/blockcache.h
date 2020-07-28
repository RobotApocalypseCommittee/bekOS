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

#ifndef BEKOS_BLOCKCACHE_H
#define BEKOS_BLOCKCACHE_H

#include <hardtypes.h>
#include <library/acquirable_ref.h>

class BlockIndexer;

struct CacheEntry {
    CacheEntry(BlockIndexer &indexer, u32 index);

    BlockIndexer& indexer;
    u32 index;

    unsigned int ref_count{};
    void* data {nullptr};
    bool dirty {false};
    bool loaded{false};

    void acquire();
    void release();
};

using CacheEntryRef = bek::AcquirableRef<CacheEntry>;

struct CacheNode;
struct InternalCacheEntry;

// Manages all CacheEntry instances, controls LRU clearing when necessary
class BlockCache {
public:

    InternalCacheEntry* registerEntry(BlockIndexer& indexer, u32 index);
    void notifyUse(InternalCacheEntry &entry);
private:
    u64 entry_count;
    u64 desired_count;
    InternalCacheEntry* first_entry;
};

// Maps ids (usually positions in file) to CacheEntry instances
class BlockIndexer {
public:
    CacheEntryRef get(u32 id);
private:
    CacheNode* splitNode(u32 id, CacheNode* current, CacheNode* parent);
    void mergeNode(int index, CacheNode* parent);
    void removeEntry(InternalCacheEntry* entry);
    void rebalance(CacheNode* node);
    CacheNode* root_entry;
    BlockCache& block_cache;
    u64 block_size;
    bek::spinlock lock;
};

#endif //BEKOS_BLOCKCACHE_H
