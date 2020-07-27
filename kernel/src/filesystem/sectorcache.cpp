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

#include <library/lock.h>
#include <kstring.h>
#include <library/utility.h>
#include "filesystem/sectorcache.h"
#include "library/array.h"

#define CHILD_COUNT 8
#define LEAF_COUNT  2

struct InternalCacheEntry {
    explicit InternalCacheEntry(CacheEntry entry) : entry(entry) {}

    CacheEntry entry;
    InternalCacheEntry *next{};
    InternalCacheEntry *prev{};

};



struct CacheNode {
    bek::arr<u64, CHILD_COUNT - 1> indices;
    bool to_leaf;
    u8 child_count;
    bek::arr<void *, CHILD_COUNT> children;
};

void addToNode(u64 id, void* ptr, CacheNode* current) {
    assert(current->child_count < CHILD_COUNT);
    int index = current->child_count - 1;
    for (int i = 0; i < current->child_count - 1; i++) {
        if (id < current->indices[i]) {
            index = i;
            // Budge over
            bek::copy_backward(current->children.data() + index + 2,
                               current->children.data() + index + 1,
                               current->child_count - 1 - index);
            bek::copy_backward(current->indices.data() + index + 1,
                               current->indices.data() + index,
                               current->child_count - 1 - index);
            break;
        }
    }
    // Add to node
    current->indices[index] = id;
    current->children[index + 1] = ptr;
    current->child_count++;
}

CacheNode* BlockCache::splitNode(u64 id, CacheNode *current, CacheNode *parent) {
    // We know it wont be less than half
    int middle_index = (current->child_count / 2) - 1;
    u64 middle_id = current->indices[middle_index];
    int left_count = middle_index + 1;
    int right_count = current->child_count - left_count;

    auto *new_node = new CacheNode;
    new_node->child_count = right_count;
    new_node->to_leaf = current->to_leaf;
    bek::copy(new_node->children.data(), current->children.data() + left_count, right_count);
    bek::copy(new_node->indices.data(), current->indices.data() + left_count, right_count - 1);

    current->child_count = left_count;

    if (parent) {
        // Max by default - we know the array is not full
        addToNode(middle_id, new_node, parent);
    } else {
        // We are splitting root node -> create new parent
        parent = new CacheNode;
        parent->child_count = 2;
        parent->to_leaf = false;
        parent->children[0] = current;
        parent->children[1] = new_node;
        parent->indices[0] = middle_id;
        root_entry = parent;
    }
    if (id < middle_id) {
        return current;
    } else {
        return new_node;
    }
}

CacheEntryRef BlockCache::get(u64 id) {
    // Lock tree
    bek::locker locker{lock};
    // Begin search
    CacheNode *parentNode = nullptr;
    CacheNode *currentNode = root_entry;
    InternalCacheEntry* result = nullptr;
    bool tentative_found = false;
    // Treats to_leaf nodes as indexed the same way
    while (currentNode != nullptr) {
        // Not there yet, descend level
        // First, check if full and split
        if (currentNode->child_count == CHILD_COUNT) {
            currentNode = splitNode(id, currentNode, parentNode);
        }
        // Find next - by default maximum
        int child_index = currentNode->child_count - 1;
        // Loop through indices
        for (int i = 0; i < currentNode->child_count - 1; i++) {
            if (id < currentNode->indices[i]) {
                child_index = i;
                break;
            }
        }
        if (!currentNode->to_leaf) {
            // Descend
            parentNode = currentNode;
            currentNode = static_cast<CacheNode *>(currentNode->children[child_index]);
        } else {
            tentative_found = true;
            result = static_cast<InternalCacheEntry*>(currentNode->children[child_index]);
            break;
        }
    }
    if (tentative_found) {
        assert (result != nullptr);
        if (result->entry.address == id) {
            // Huzzah
            return CacheEntryRef(&result->entry);
        } else {
            if (entry_count < desired_count) {
                // Go ahead create more
                entry_count++;
                auto* entry = createEntry(id);
                addToNode(id, entry, currentNode);
                return CacheEntryRef(&result->entry);
            } else {
                // TODO: Flush or something
            }
        }
    } else {
        // This shouldn't be possible
        // TODO: Neaten
        assert(0);
    }
}

InternalCacheEntry *BlockCache::createEntry(u64 id) {
    CacheEntry new_entry;
    new_entry.address = id;
    // TODO: Block allocator AAAAA
    new_entry.data = new u8[block_size];
    new_entry.dirty = false;
    new_entry.loaded = false;
    new_entry.ref_count = 0;
    auto* n = new InternalCacheEntry(new_entry);
    n->next = first_entry;
    first_entry->prev = n;
    first_entry = n;
    return n;
}

void CacheEntry::acquire() {
    ref_count++;
}

void CacheEntry::release() {
    ref_count--;
}
