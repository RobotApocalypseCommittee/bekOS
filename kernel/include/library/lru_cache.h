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

#ifndef BEKOS_LRU_CACHE_H
#define BEKOS_LRU_CACHE_H

#include "debug.h"

/// Least recently used cache of objects. Val should be small and easily copyable. The cache will attempt to purge each
/// item
/// \tparam Key
/// \tparam Val
template <typename Key, bek::ref_counted Val>
class LRUCache {
public:
    using ValPtr = bek::shared_ptr<Val>;
    using PurgeFn = bek::function<void(Key, ValPtr)>;

private:
    using DBG = DebugScope<"LRU", DebugLevel::WARN>;
    struct Item {
        u64 last_use_counter;
        ValPtr v;
    };
    using Hashtable = bek::hashtable<Key, Item>;

public:
    LRUCache(uSize max_items, PurgeFn purge_fn)
        : m_counter{0}, m_max_items{max_items}, m_hashtable{max_items * 2}, m_purge_fn{bek::move(purge_fn)} {}

    /// Attempts to insert item into cache. Returns true if successful, and false if item already exists.
    bool set(Key k, ValPtr v) {
        auto [it, success] = m_hashtable.insert({k, {m_counter, bek::move(v)}});
        try_purge();
        return success;
    }

    /// Tries to find item with key.
    /// \param key
    /// \return Pointer if found. *May be nullptr*.
    ValPtr find(const Key& key) {
        // TODO: Locking
        auto* x = m_hashtable.find(key);
        if (x) {
            x->last_use_counter = ++m_counter;
            return x->v;
        }
        return nullptr;
    }

private:
    void try_purge() {
        if (m_hashtable.item_count() > m_max_items) {
            // Loop through, finding which (a) was least recently used and (b) is currently unreferenced.

            // (key, last use counter)
            bek::optional<bek::pair<Key, u64>> to_delete{};

            // TODO: Lock
            for (auto& item : m_hashtable) {
                if (item.second.v->ref_count() == 1 &&
                    (!to_delete || to_delete->second > item.second.last_use_counter)) {
                    to_delete = bek::pair{item.first, item.second.last_use_counter};
                }
            }

            if (to_delete) {
                // Inefficient - performs (in most cases) a double lookup.
                auto x = m_hashtable.extract(to_delete->first);
                if (x) {
                    // Now, we need to double-check refcount - depends on locking.
                    if (x->v->ref_count() == 1) {
                        m_purge_fn(to_delete->first, bek::move(x->v));
                    } else {
                        DBG::dbgln("Disaster: Whilst purging, a reference to object was obtained!"_sv);
                        m_hashtable.set(to_delete->first, *bek::move(x));
                    }
                }
            } else {
                DBG::dbgln("Warning: Could not find cache item to purge."_sv);
            }
        }
    }

    u64 m_counter;
    uSize m_max_items;
    Hashtable m_hashtable;
    PurgeFn m_purge_fn;
};

#endif  // BEKOS_LRU_CACHE_H
