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

extern "C" {
extern u8 __kernel_start, __kernel_end;
}

bek::optional<mem::PhysicalPtr> mem::kernel_virt_to_phys(void* ptr) {
    auto virt = reinterpret_cast<uPtr>(ptr);
    u64 par_value;
    asm("AT S1E1R, %1; MRS %0, PAR_EL1" : "=r"(par_value) : "r"(virt & PAGE_MASK));
    if (par_value & 1) return {};
    uPtr raw_phys = par_value & 0x0000FFFFFFFFF000;
    return {raw_phys | (virt & PAGE_OFFSET_MASK)};
}

void* mem::kernel_phys_to_virt(mem::PhysicalPtr ptr) {
    uSize kernel_size      = &__kernel_end - &__kernel_start;
    auto kernel_start_phys = *mem::kernel_virt_to_phys(&__kernel_start);
    mem::PhysicalRegion kernel_region{kernel_start_phys, kernel_size};
    if (kernel_region.contains(ptr)) {
        return &__kernel_start + (ptr.get() - kernel_start_phys.get());
    } else {
        return reinterpret_cast<void*>(ptr.get() + VA_IDENT_OFFSET);
    }
}

u64 get_cache_line_size() {
    // CTR_EL0 - §D13.2.34
    u64 ctr_reg;
    asm("mrs %0, CTR_EL0" : "=r"(ctr_reg));
    return 4 << ((ctr_reg >> 16) & 0xF);
}

extern "C" {
void asm_arm64_clean_cache(u64 start, u64 end, u64 line_sz);
void asm_arm64_invalidate_cache(u64 start, u64 end, u64 line_sz);
}

// FIXME: When aligning, could invalidate some memory still to be written.
void mem::dma_sync_before_read(const void* ptr, uSize size) {
    auto line_sz = get_cache_line_size();

    // Align start downwards
    uPtr start = reinterpret_cast<uPtr>(ptr) & (1 - line_sz);
    // Want to align up.
    uPtr end = (reinterpret_cast<uPtr>(ptr) + size + line_sz - 1) & (1 - line_sz);
    asm_arm64_invalidate_cache(start, end, line_sz);
}
void mem::dma_sync_after_write(const void* ptr, uSize size) {
    auto line_sz = get_cache_line_size();

    // Align start downwards
    uPtr start = reinterpret_cast<uPtr>(ptr) & (1 - line_sz);
    // Want to align up.
    uPtr end = (reinterpret_cast<uPtr>(ptr) + size + line_sz - 1) & (1 - line_sz);
    asm_arm64_clean_cache(start, end, line_sz);
}
