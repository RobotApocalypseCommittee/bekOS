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

#include "mm/dma_utils.h"

namespace mem {

mem::own_dma_buffer MappedDmaPool::allocate(uSize size, uSize align) {
    auto allocation = kmalloc(size, align);
    VERIFY(allocation);
    auto raw_ptr = mem::kernel_virt_to_phys(allocation);
    VERIFY(raw_ptr);
    for (auto& mapping : m_mappings) {
        mem::PhysicalPtr region_start{mapping.parent_address};
        if (region_start <= *raw_ptr && *raw_ptr <= region_start.offset(mapping.size)) {
            // In this region!
            mem::DmaPtr dma_region_start{mapping.child_address};
            auto dma_ptr = dma_region_start.offset(raw_ptr->get() - region_start.get());
            return {*this, {(char*)allocation, size, dma_ptr}, align};
        }
    }
    ASSERT_UNREACHABLE();
}
void MappedDmaPool::deallocate(const mem::own_dma_buffer& buffer) {
    kfree(buffer.data(), buffer.size(), buffer.align());
}

}  // namespace mem
