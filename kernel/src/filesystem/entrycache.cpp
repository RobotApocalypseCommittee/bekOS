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

#include "filesystem/entrycache.h"

fs::EntryRef fs::EntryHashtable::search(EntryRef root, const char* name) {
    u64 hash = entry_hash(root->get_hash(), name) % ENTRY_HASHTABLE_BUCKETS;
    auto* x = buckets[hash];
    while (x != nullptr) {
        if (strcmp(x->filesystemEntry->name.data(), name) == 0 && root == x->filesystemEntry->parent) {
            // Identical
            return x->filesystemEntry;
        }
        x = x->next;
    }
    return EntryRef();
}

void fs::EntryHashtable::insert(EntryRef entry) {
    u64 hash = entry->get_hash() % ENTRY_HASHTABLE_BUCKETS;
    buckets[hash] = new HashtableEntry { entry, buckets[hash]};
}

void fs::EntryHashtable::remove(EntryRef ref) {
    auto* x = buckets[ref->get_hash() % ENTRY_HASHTABLE_BUCKETS];
    if (x != nullptr) {
        if (x->filesystemEntry == ref) {
            // Comparing by pointer is ok, cos there'll only ever be one entry in memory corresponding to the actual entry.
            buckets[ref->get_hash() % ENTRY_HASHTABLE_BUCKETS] = x->next;
            delete x;
        } else {
            auto prev = x;
            x = x->next;
            while (x != nullptr) {
                if (x->filesystemEntry == ref) {
                    prev->next = x->next;
                    delete x;
                    break;
                }
            }
        }
    }
}

void fs::EntryHashtable::clean() {
    for (auto & bucket : buckets) {
        auto* x = bucket;
        auto* prev_ptr = &bucket;
        while (x != nullptr) {
            if (x->filesystemEntry.unique()) {
                // Delete me
                auto ptr =  x->filesystemEntry.get();
                *prev_ptr = x->next;
                delete x;
                delete ptr;
                x = *prev_ptr;
            } else {
                // Progress
                prev_ptr = &x->next;
                x = x->next;
            }
        }
    }
}
