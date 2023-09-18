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

#include "filesystem/filesystem.h"

#include <utility>
#include "printf.h"

void fs::Entry::acquire() {
    // TODO: Safety
    ref_count++;
    printf("Acquired: %s, %d\n", name.data(), ref_count);
}

void fs::Entry::release() {
    // TODO: Safety
    ref_count--;
    printf("Released: %s, %d\n", name.data(), ref_count);
    if (_dirty && ref_count == 0) {
        printf("Committing changes to %s\n", name.data());
        commit_changes();
    }
}

void fs::Entry::setParent(EntryRef newParent) {
    parent = std::move(newParent);
}

u64 fs::Entry::get_hash() {
    if (m_hash == 0) {
        m_hash = entry_hash(parent.get() != nullptr ? parent->get_hash() : 0, name.data());
    }
    return m_hash;
}

unsigned fs::Entry::get_ref_count() const {
    return ref_count;
}

fs::Entry::~Entry() {
    printf("Deleting %s\n", name.data());
}

void fs::Entry::mark_dirty() {
    _dirty = true;
}

bek::vector<fs::EntryRef> fs::Entry::enumerate() { ASSERT(false && "Should be implemented"); }

fs::EntryRef fs::Entry::lookup(bek::string_view name) {
    (void) name;
    ASSERT(false && "Should be implemented");
}


u64 fs::entry_hash(u64 previous, const char *name) {
    u64 hash;
    if (previous == 0) {
        hash = 5381;
    } else {
        hash = previous;
    }
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5u) + hash) + c;
    }
    return hash;
}

fs::EntryRef fs::fullPathLookup(fs::EntryRef root, bek::string_view s) {
    for (uSize i = 0; i < s.size(); i++) {
        if (s.data()[i] == '/') {
            auto currentPiece = s.substr(0, i);
            s.remove_prefix(i + 1);
            if (root->type == EntryType::DIRECTORY) {
                root = root->lookup(currentPiece);
            } else {
                return {};
            }
        }
    }
    return root->type == EntryType::DIRECTORY ? root->lookup(s) : EntryRef{};
}


fs::Filesystem::Filesystem(fs::EntryHashtable &entryCache) : entryCache(entryCache) {
}
