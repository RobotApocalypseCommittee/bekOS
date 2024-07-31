/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_BLOCKCACHE_H
#define BEKOS_BLOCKCACHE_H

#include <library/lock.h>

#include "bek/types.h"

class BlockIndexer;

struct CacheEntry : public bek::Acquirable<CacheEntry> {
    CacheEntry(BlockIndexer &indexer, u32 index);

    BlockIndexer &indexer;
    u64 index;

    void *data{nullptr};
    bool dirty{false};
    bool loaded{false};
};

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
    BlockIndexer(BlockCache &blockCache, u64 blockSize);

    CacheEntry::ref get(u64 id);
private:
    CacheNode* splitNode(u64 id, CacheNode* current, CacheNode* parent);
    void mergeNode(int index, CacheNode* parent);
    void removeEntry(InternalCacheEntry* entry);
    void rebalance(CacheNode* node);
    CacheNode* root_entry;
    BlockCache& block_cache;
    u64 block_size;
    bek::spinlock lock;
};

#endif //BEKOS_BLOCKCACHE_H
