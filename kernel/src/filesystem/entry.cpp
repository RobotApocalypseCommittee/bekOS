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

#include "filesystem/entry.h"

#include "filesystem/filesystem.h"
#include "library/debug.h"

using DBG = DebugScope<"FS", false>;

u64 fs::Entry::get_hash() {
    if (m_hash == 0) {
        m_hash = bek::hash(m_name);
        if (auto p = parent(); p) {
            auto parent_hash = p->get_hash();
            m_hash ^= parent_hash + 0x9e3779b9 + (m_hash << 6) + (m_hash >> 2);
        }
    }
    return m_hash;
}

fs::Entry::~Entry() { DBG::dbgln("Deleting {}"_sv, m_name.view()); }
void fs::Entry::set_timestamps(const fs::EntryTimestamps& timestamps) {
    if (timestamps.accessed && timestamps.accessed != m_timestamps.accessed) {
        m_timestamps.accessed = timestamps.accessed;
        m_dirty = true;
    }
    if (timestamps.modified && timestamps.modified != m_timestamps.modified) {
        m_timestamps.modified = timestamps.modified;
        m_dirty = true;
    }
    if (timestamps.created && timestamps.created != m_timestamps.created) {
        m_timestamps.created = timestamps.created;
        m_dirty = true;
    }
}

fs::Entry::Entry(bool is_directory, bek::string name, fs::EntryTimestamps timestamps, uSize size)
    : m_name(bek::move(name)), m_timestamps(timestamps), m_size(size), m_is_directory(is_directory) {}
