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

#include "filesystem/sectorcache.h"

struct InternalCacheEntry {
    CacheEntry entry;
    InternalCacheEntry* left{};
    InternalCacheEntry* right{};
    InternalCacheEntry* parent{};
    InternalCacheEntry* next;
    bool black;

    InternalCacheEntry(const CacheEntry &entry, InternalCacheEntry *next) : entry(entry), next(next), black(false) {}
};

InternalCacheEntry* get_sibling(InternalCacheEntry* node) {
    if (node->parent == nullptr) return nullptr;
    if (node == node->parent->left) {
        return node->parent->right;
    } else {
        return node->parent->left;
    }

}

void rotate_left(InternalCacheEntry* root) {
    InternalCacheEntry* pivot = root->right;
    InternalCacheEntry* parent = root->parent;
    assert(pivot != nullptr);

    auto* oldleft = pivot->left;

    pivot->left = root;
    root->parent = pivot;

    root->right = oldleft;
    if (oldleft != nullptr) {
        oldleft->parent = root;
    }

    pivot->parent = parent;
    if (parent != nullptr) {
        if (root == parent->left) {
            parent->left = pivot;
        } else {
            parent->right = pivot;
        }
    }
}

void rotate_right(InternalCacheEntry* root) {
    InternalCacheEntry* pivot = root->left;
    InternalCacheEntry* parent = root->parent;
    assert(pivot != nullptr);

    auto* oldright = pivot->right;

    pivot->right = root;
    root->parent = pivot;

    root->right = oldright;
    if (oldright != nullptr) {
        oldright->parent = root;
    }

    pivot->parent = parent;
    if (parent != nullptr) {
        if (root == parent->left) {
            parent->left = pivot;
        } else {
            parent->right = pivot;
        }
    }
}

void SectorCache::addEntry(CacheEntry entry) {
    if (entry_count >= desired_count) {
        // TODO: Remove last
    }
    auto* new_entry = new InternalCacheEntry(entry, first_entry);
    first_entry = new_entry;

    // Insert into tree
    InternalCacheEntry* current_root = root_entry;
    while (current_root != nullptr) {
        if (new_entry->entry.entryID < current_root->entry.entryID) {
            if (current_root->left == nullptr) {
                current_root->left = new_entry;
                new_entry->parent = current_root;
                break;
            } else {
                current_root = current_root->left;
                continue; // Not strictly necessary
            }
        } else {
            if (current_root->right == nullptr) {
                current_root->right = new_entry;
                new_entry->parent = current_root;
                break;
            } else {
                current_root = current_root->right;
                continue;
            }
        }
    }
    insertionRepair(new_entry);
}

void SectorCache::insertionRepair(InternalCacheEntry *node) {
    if (node->parent == nullptr) {
        // Root node
        node->black = true;
    } else if (node->parent->black) {
        // Do nothing
    } else if (get_sibling(node->parent) != nullptr && !get_sibling(node->parent)->black) {
        // Make both parents black, propagate
        node->parent->black = true;
        get_sibling(node->parent)->black = true;
        // TODO: Recursive - potentially sticky
        // If there is uncle, there is grandparent
        insertionRepair(node->parent->parent);
    } else {
        // parent is red, parent's sibling is black
        auto* parent = node->parent;
        auto* grandparent = parent->parent; // Parent not black, so grandparent not null
        bool grandparent_root = grandparent->parent == nullptr;

        auto* c_node = node;
        // If new is 'inside', move it to parent's place
        if (node == parent->right && parent == grandparent->left) {
            rotate_left(parent);
            bek::swap(parent, c_node);
        } else if (node == parent->left && parent == grandparent->right) {
            rotate_right(parent);
            bek::swap(parent, c_node);
        }

        if (c_node == parent->left) {
            rotate_right(grandparent);
        } else {
            rotate_left(grandparent);
        }
        parent->black = true;
        grandparent->black = false;
        if (grandparent_root) {
            root_entry = parent;
        }
    }

}

CacheEntry* SectorCache::lookup(u64 id) {
    auto* current_root = root_entry;
    while (current_root != nullptr) {
        if (id == current_root->entry.entryID) {
            return &current_root->entry;
        } else if (id < current_root->entry.entryID) {
            current_root = current_root->left;
        } else {
            current_root = current_root->right;
        }
    }
    return nullptr;
}
