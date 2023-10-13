/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_HASHTABLE_H
#define BEKOS_HASHTABLE_H

#include "kstring.h"
#include "mm/kmalloc.h"
#include "optional.h"
#include "types.h"
#include "utility.h"
namespace bek {

namespace detail {
template <typename T>
concept has_bek_hash_overload = requires(const T& t) {
    { hash(t) } -> same_as<u64>;
};
}

template <typename T>
struct hasher;

template <detail::has_bek_hash_overload T>
struct hasher<T> {
    u64 operator()(const T& x) const noexcept {
        using namespace bek;
        return hash(x);
    }
};

template <typename Key, typename Val, typename Hasher = bek::hasher<Key>>
class hashtable {
private:
    enum class Fill : u8 { Empty, Deleted, Filled };

public:
    using Pair     = pair<Key, Val>;
    using Iterator = Pair*;

    explicit hashtable(uSize capacity = 4)
        : buckets{nullptr}, filled{nullptr}, capacity{capacity}, load{0} {
        buckets = reinterpret_cast<Pair*>(kmalloc(capacity * sizeof(Pair)));
        filled  = reinterpret_cast<Fill*>(kmalloc(capacity * sizeof(Fill)));
        for (uSize i = 0; i < capacity; i++) {
            filled[i] = Fill::Empty;
        }
    };

    pair<Iterator, bool> insert(const Pair& pair)
        requires copy_constructible<Pair>
    {
        check_size();
        return unchecked_set(pair, false);
    }

    pair<Iterator, bool> insert(Pair&& pair) {
        check_size();
        return unchecked_set(bek::move(pair), false);
    }

    void set(Key key, Val val) {
        check_size();
        return unchecked_set({key, val}, true);
    }

    Val* find(const Key& key) {
        auto index = unchecked_lookup(key, false);
        if (filled[index] == Fill::Filled) {
            return &(buckets[index].second);
        } else {
            return nullptr;
        }
    }

    const Val* find(const Key& key) const {
        auto index = unchecked_lookup(key, false);
        if (filled[index] == Fill::Filled) {
            return &(buckets[index].second);
        } else {
            return nullptr;
        }
    }

    /// Tries to remove item from table, returning it.
    /// \param key
    /// \return
    bek::optional<Val> extract(const Key& key) {
        auto index = unchecked_lookup(key, false);
        if (filled[index] != Fill::Filled) return {};
        // We found something, so have to remove and mark as removed.
        Val ret = bek::move(buckets[index].second);
        buckets[index].~Pair();
        filled[index] = Fill::Deleted;
        load--;
        return ret;
    }

    const Val& find_unchecked(const Key& key) const { return *find(key); }

    Val& find_unchecked(const Key& key) { return *find(key); }

private:
    void check_size() {
        if (load * expandThreshold > capacity) {
            auto new_size = capacity * expandFactor;
            // Need to expand + rehash
            auto new_buckets = reinterpret_cast<Pair*>(kmalloc(new_size * sizeof(Pair)));
            auto new_filled  = reinterpret_cast<Fill*>(kmalloc(new_size * sizeof(Fill)));
            memset(new_filled, 0, new_size * sizeof(bool));

            // Swap
            auto old_buckets  = buckets;
            buckets           = new_buckets;
            auto old_filled   = filled;
            filled            = new_filled;
            auto old_capacity = capacity;
            capacity          = new_size;
            load              = 0;

            // Transfer
            for (uSize i = 0; i < old_capacity; i++) {
                if (old_filled[i] == Fill::Filled) {
                    unchecked_set(bek::move(old_buckets[i]), false);
                    old_buckets[i].~Pair();
                }
            }

            // Delete
            kfree(old_buckets, old_capacity * sizeof(Pair));
            kfree(old_filled, old_capacity * sizeof(bool));
        }
    }

    pair<Iterator, bool> unchecked_set(Pair&& pair, bool overwrite) {
        auto index  = unchecked_lookup(pair.first, true);
        Iterator it = &buckets[index];
        if (filled[index] == Fill::Filled) {
            if (overwrite) {
                *it = bek::move(pair);
                return {it, true};
            } else {
                return {it, false};
            }
        } else {
            new (it) Pair{bek::move(pair)};
        }
        load += filled[index] == Fill::Filled ? 0 : 1;
        filled[index] = Fill::Filled;
        return {it, true};
    }

    /// Looks for Key. If found, returns index. If not, returns appropriate insertion point.
    uSize unchecked_lookup(const Key& key, bool inserting) const {
        auto hash   = Hasher{}(key);
        uSize index = hash % capacity;
        for (uSize i = index; i < capacity; i++) {
            if (filled[i] == Fill::Empty || (filled[i] == Fill::Deleted && inserting) ||
                (filled[i] == Fill::Filled && key == buckets[i].first)) {
                return i;
            }
        }
        for (uSize i = 0; i < index; i++) {
            if (filled[i] == Fill::Empty || (filled[i] == Fill::Deleted && inserting) ||
                (filled[i] == Fill::Filled && key == buckets[i].first)) {
                return i;
            }
        }
        ASSERT_UNREACHABLE();
    }

    static constexpr int expandThreshold = 2;
    static constexpr int expandFactor    = 2;

    /// Dynamically Allocated array of buckets.
    Pair* buckets;
    /// Dynamically Allocated bitmap of filled flags.
    Fill* filled;

    /// Length of array allocated
    uSize capacity;
    /// How many items?
    uSize load;
};

};  // namespace bek

#endif  // BEKOS_HASHTABLE_H
