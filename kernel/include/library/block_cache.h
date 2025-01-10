// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_BLOCK_CACHE_H
#define BEKOS_BLOCK_CACHE_H

#include "bek/types.h"
#include "bek/intrusive_shared_ptr.h"

class BlockCacheItem final : public bek::RefCounted<BlockCacheItem> {
public:
    static bek::shared_ptr<BlockCacheItem> create(u32 size);
    u8* data() const { return m_buffer; }
    u32 size() const { return m_size; }
    bool is_dirty() const { return (m_dirty_end - m_dirty_start) != 0; }
    bek::pair<u32, u32> dirty_range() const { return {m_dirty_start, m_dirty_end}; }
    void add_dirty_region(u32 start, u32 end) {
        VERIFY(start <= end && end <= m_size);
        if (is_dirty()) {
            m_dirty_start = bek::min(start, m_dirty_start);
            m_dirty_end = bek::max(end, m_dirty_end);
        } else {
            m_dirty_start = start;
            m_dirty_end = end;
        }
    }

    void clear_dirty() {
        m_dirty_start = 0;
        m_dirty_end = 0;
    }

    void set_whole_dirty() {
        m_dirty_end = m_size;
        m_dirty_start = 0;
    }
    ~BlockCacheItem();

    BlockCacheItem(const BlockCacheItem&) = delete;
    BlockCacheItem& operator=(const BlockCacheItem&) = delete;

private:
    BlockCacheItem(u8* buffer, u32 size) : m_buffer{buffer}, m_size{size} {}
    u8* m_buffer;
    u32 m_size;
    u32 m_dirty_start{};
    u32 m_dirty_end{};
};

#endif  // BEKOS_BLOCK_CACHE_H
