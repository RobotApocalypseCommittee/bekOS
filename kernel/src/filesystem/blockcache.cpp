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

#include "filesystem/blockcache.h"

#include <c_string.h>
#include <library/lock.h>

#include "bek/utility.h"
#include "library/array.h"

CacheEntry::CacheEntry(BlockIndexer &indexer, u32 index) : indexer(indexer), index(index) {}

struct InternalCacheEntry {
    explicit InternalCacheEntry(CacheEntry entry) : entry(entry) {}

    CacheEntry entry;
    InternalCacheEntry *next{};
    InternalCacheEntry *prev{};
    CacheNode *parent{};
};

#define CHILD_COUNT 16
#define CHILD_COUNT_MIN 8
#define LEAF_COUNT  2

struct CacheNode {
    bek::array<u32, CHILD_COUNT - 1> indices;
    bool to_leaf;
    u8 child_count;
    CacheNode* parent;
    bek::array<void *, CHILD_COUNT> children;
};

void addToNode(u32 id, void *ptr, CacheNode *current) {
    ASSERT(current->child_count < CHILD_COUNT);
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

bek::pair<u32, void*> stealFromNode(int index, CacheNode* node, bool right) {
    // Looking the correct way
    ASSERT(index != 0 || right);
    ASSERT(index != node->child_count - 1 || !right);

    bek::pair<u32, void*> retval{node->indices[right ? index : index - 1], node->children[index]};
    if (index + 1 < node->child_count) {
        // Not the last
        // Shift left
        bek::copy(node->children.data() + index, node->children.data() + index + 1, node->child_count - index - 1);
        bek::copy(node->indices.data() + index - (right ? 0 : 1), node->indices.data() + index + (right ? 1 : 0), node->child_count - index - (right ? 2 : 1));
    }
    node->child_count--;
    return retval;
}

CacheNode *BlockIndexer::splitNode(u64 id, CacheNode *current, CacheNode *parent) {
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
    new_node->parent = parent;

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
        current->parent = parent; new_node->parent = parent;
        root_entry = parent;
    }
    if (id < middle_id) {
        return current;
    } else {
        return new_node;
    }
}

CacheEntry::ref BlockIndexer::get(u64 id) {
    // Lock tree
    bek::locker locker{lock};
    // Begin search
    CacheNode *parentNode = nullptr;
    CacheNode *currentNode = root_entry;
    InternalCacheEntry *result = nullptr;
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
            result = static_cast<InternalCacheEntry *>(currentNode->children[child_index]);
            break;
        }
    }
    assert (result != nullptr);
    if (result->entry.index == id) {
        // Huzzah
        block_cache.notifyUse(*result);
        return &result->entry;
    } else {
        auto *entry = block_cache.registerEntry(*this, id);
        if (entry) {
            entry->parent = currentNode;
            addToNode(id, entry, currentNode);
            return &result->entry;
        }
        return nullptr;
    }
}

void BlockIndexer::removeEntry(InternalCacheEntry *entry) {
    ASSERT(entry);
    auto* parent = entry->parent;
    ASSERT(parent != nullptr);
    // Find
    int index = -1;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == entry) {
            index = i;
            break;
        }
    }
    ASSERT(index != -1);
    // Delete element
    stealFromNode(index, parent, index == 0);
    rebalance(parent);
    // Note the InternalCacheEntry still exists
    entry->parent = nullptr;
}

void BlockIndexer::mergeNode(int index, CacheNode *parent) {
    // Merge index and index + 1
    auto* left = static_cast<CacheNode*>(parent->children[index]);
    auto* right = static_cast<CacheNode*>(parent->children[index + 1]);
    auto [sep, _] = stealFromNode(index + 1, parent, false);
    left->indices[left->child_count - 1] = sep;
    bek::copy(left->indices.data() + left->child_count, right->indices.data(), right->child_count - 1);
    bek::copy(left->children.data() + left->child_count, right->children.data(), right->child_count);
    left->child_count += right->child_count;
    // Groovy
    delete right;
}

void BlockIndexer::rebalance(CacheNode *node) {
    if (node->child_count < CHILD_COUNT_MIN) {
        if (auto* parent = node->parent) {
            // Find
            int index = -1;
            for (int i = 0; i < parent->child_count; i++) {
                if (parent->children[i] == node) {
                    index = i;
                    break;
                }
            }
            ASSERT(index != -1);
            if (index + 1 < parent->child_count && static_cast<CacheNode*>(parent->children[index + 1])->child_count > CHILD_COUNT_MIN) {
                // steal from right sibling
                auto* right_sibling = static_cast<CacheNode*>(parent->children[index + 1]);
                auto [sep, child] = stealFromNode(0, right_sibling, true);
                addToNode(parent->indices[index], child, node);
                parent->indices[index] = sep;
            } else if (index > 0 && static_cast<CacheNode*>(parent->children[index - 1])->child_count > CHILD_COUNT_MIN) {
                // steal from left sibling
                auto* left_sibling = static_cast<CacheNode*>(parent->children[index - 1]);
                auto [sep, child] = stealFromNode(left_sibling->child_count - 1, left_sibling, false);
                addToNode(parent->indices[index], child, node);
                parent->indices[index] = sep;
            } else {
                // We need to merge
                mergeNode(index == 0 ? 0 : index - 1, parent);
                // Could have affected parent
                rebalance(parent);
            }
        } else {
            // Root node - check for emptyness
            if (node->child_count == 1 && !node->to_leaf) {
                // Delete
                root_entry = static_cast<CacheNode *>(node->children[0]);
                delete node;
            }
        }
    }
}

BlockIndexer::BlockIndexer(BlockCache &blockCache, u64 blockSize) : block_cache(blockCache), block_size(blockSize) {}

InternalCacheEntry *BlockCache::registerEntry(BlockIndexer &indexer, u32 index) {
    // TODO: Better allocator?
    auto *new_entry = new InternalCacheEntry(CacheEntry(indexer, index));
    if (entry_count >= desired_count) {
        // TODO: Flush, erase
    }
    entry_count++;
    // TODO: Thread-safe?
    new_entry->next = first_entry;
    first_entry->prev = new_entry;
    first_entry = new_entry;
    return new_entry;
}

void BlockCache::notifyUse(InternalCacheEntry &entry) {
    // Move to front of list
    // TODO: Thread-safe
    auto *n = entry.next;
    auto *p = entry.prev;
    if (p) {
        // Isn't already
        p->next = n;
        if (n) {
            n->prev = p;
        }
        // Now insert
        entry.next = first_entry;
        first_entry->prev = &entry;
        first_entry = &entry;
    }
}
