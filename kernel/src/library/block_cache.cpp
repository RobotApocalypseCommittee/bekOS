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

#include "library/block_cache.h"

#include "bek/allocations.h"

bek::shared_ptr<BlockCacheItem> BlockCacheItem::create(u32 size) {
    // TODO: Custom new and delete? - allocate in one block
    auto allocation = mem::allocate(size);
    VERIFY(allocation.pointer);
    return {bek::shared_ptr<BlockCacheItem>::AdoptTag{},
            new BlockCacheItem(static_cast<u8*>(allocation.pointer), size)};
}

BlockCacheItem::~BlockCacheItem() {
    VERIFY(!is_dirty());
    mem::free(m_buffer, m_size);
}
