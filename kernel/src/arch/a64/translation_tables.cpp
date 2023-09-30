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

#include <arch/a64/translation_tables.h>

#include "library/debug.h"

using DBG = DebugScope<"PGTBL", true>;

#define L0_SHIFT (39)
#define L1_SHIFT (30)
#define L2_SHIFT (21)
#define L3_SHIFT (12)

#define L0_SIZE (1ull << L0_SHIFT)
#define L1_SIZE (1ull << L1_SHIFT)
#define L2_SIZE (1ull << L2_SHIFT)
#define L3_SIZE (1ull << L3_SHIFT)

#define PT_INDEX_MASK (512 - 1)
#define PT_ENTRY_COUNT (512)

#define PT_UPPER_BLOCK_DESCRIPTOR (0b01)
#define PT_UPPER_TABLE_DESCRIPTOR (0b11)
#define PT_LOWER_BLOCK_DESCRIPTOR (0b11)
#define PT_INVALID_DESCRIPTOR (0b00)

constexpr inline uSize SHIFTS[] = {39, 30, 21, 12};
constexpr inline uSize SIZES[]  = {1ull << 39, 1ull << 30, 1ull << 21, 1ull << 12};

#define PAGE_OFFSET_MASK ((1ul << 12) - 1)

extern "C" {
extern u8 __initial_pgtables_start, __initial_pgtables_end;
}

/*

bool CrudeTableManager::map_region(uPtr virtual_address, uPtr phys_address, uSize size,
                                   PageAttributes attributes, MemAttributeIndex idx) {
    if (!m_l0_table) return false;
    auto ret = map_upper_level(virtual_address, phys_address, size, attributes, idx, m_l0_table,
L0); m_pages_cache.refresh_cache(); return ret;

}
bool CrudeTableManager::map_upper_level(uPtr& virt_address, uPtr& phys_address, uSize& size,
                                        PageAttributes attributes, MemAttributeIndex attr_idx,
                                        ARMv8MMU_UpperEntry* table, TableLevel level) {

}
bool CrudeTableManager::map_l3_table(uPtr& virt_address, uPtr& phys_address, uSize& size,
                                     PageAttributes attributes, MemAttributeIndex attr_idx,
                                     ARMv8MMU_L3_Entry* table) {

}
*/

struct [[gnu::packed]] ARMv8MMU_L3_Entry {
    u64 descriptor_code : 2, lower_attrs : 10, address : 36, res0 : 3, upper_attrs : 13;

    static constexpr ARMv8MMU_L3_Entry create(uPtr phys_addr, u64 flags) {
        union {
            u64 raw{0};
            ARMv8MMU_L3_Entry entry;
        } ret;
        ret.raw                   = flags;
        ret.entry.address         = phys_addr >> PAGE_SHIFT;
        ret.entry.descriptor_code = PT_LOWER_BLOCK_DESCRIPTOR;
        return ret.entry;
    }

    friend bool operator==(ARMv8MMU_L3_Entry a, ARMv8MMU_L3_Entry b) = default;
};

struct [[gnu::packed]] ARMv8MMU_Upper_Entry_Table {
    u64 descriptor_code : 2, ignored_0 : 10, table_page : 36, res0 : 4, ignored : 7, attrs : 5;
};

struct [[gnu::packed]] ARMv8MMU_Upper_Entry_Block {
    u64 descriptor_code : 2, lower_attrs : 10, res0_0 : 4, nT : 1, res0_1 : 4, address : 27,
        res0_2 : 4, upper_attrs : 12;
};

union ARMv8MMU_UpperEntry {
    u64 raw;
    ARMv8MMU_Upper_Entry_Block block;
    ARMv8MMU_Upper_Entry_Table table;

    static constexpr ARMv8MMU_UpperEntry create_table_entry(mem::PhysicalPtr next_table) {
        ARMv8MMU_UpperEntry entry{};
        entry.table.descriptor_code = PT_UPPER_TABLE_DESCRIPTOR;
        entry.table.table_page      = next_table.get() >> PAGE_SHIFT;
        return entry;
    }
    static constexpr ARMv8MMU_UpperEntry create_block_entry(uPtr raw_ptr, u64 flags) {
        ARMv8MMU_UpperEntry entry{};
        entry.raw                   = flags;
        entry.block.descriptor_code = PT_UPPER_BLOCK_DESCRIPTOR;
        entry.block.address         = raw_ptr >> SHIFTS[2];
        return entry;
    }
};
static_assert(sizeof(ARMv8MMU_UpperEntry) == 8);
static_assert(sizeof(ARMv8MMU_L3_Entry) == 8);

bool crude_map_region(uPtr virt_addr, uPtr phys_addr, uSize size, u64 flags, uPtr tables_start,
                      uPtr& tables_current, uPtr tables_end) {
    // Check
    if (virt_addr & (L2_SIZE - 1) || phys_addr & (L2_SIZE - 1) || size & (L2_SIZE - 1))
        return false;
    // Cannot deal with going over L2 table boundary.
    if ((virt_addr >> L1_SHIFT) != ((virt_addr + size) >> L1_SHIFT)) return false;

    if (tables_current == tables_start) {
        // Need to init whole space.
        memset(reinterpret_cast<void*>(tables_start), 0, tables_end - tables_start);
        tables_current += PAGE_SIZE;
    }

    // --- L0 Table ---
    auto l0_idx     = (virt_addr >> L0_SHIFT) & PT_INDEX_MASK;
    auto* l0_tbl    = reinterpret_cast<ARMv8MMU_Upper_Entry_Table*>(tables_start);
    auto l1_tbl_ptr = l0_tbl[l0_idx].table_page << PAGE_SHIFT;
    if (l0_tbl[l0_idx].descriptor_code == PT_INVALID_DESCRIPTOR) {
        // Ptr above is invalid!
        if (tables_current < tables_end) {
            l1_tbl_ptr                     = tables_current;
            l0_tbl[l0_idx].table_page      = tables_current >> PAGE_SHIFT;
            l0_tbl[l0_idx].descriptor_code = PT_UPPER_TABLE_DESCRIPTOR;
            tables_current += PAGE_SIZE;
        } else {
            return false;
        }
    }

    auto l1_idx     = (virt_addr >> L1_SHIFT) & PT_INDEX_MASK;
    auto* l1_tbl    = reinterpret_cast<ARMv8MMU_Upper_Entry_Table*>(l1_tbl_ptr);
    auto l2_tbl_ptr = l1_tbl[l1_idx].table_page << PAGE_SHIFT;
    if (l1_tbl[l1_idx].descriptor_code == PT_INVALID_DESCRIPTOR) {
        // Ptr above is invalid!
        if (tables_current < tables_end) {
            l2_tbl_ptr                     = tables_current;
            l1_tbl[l1_idx].table_page      = tables_current >> PAGE_SHIFT;
            l1_tbl[l1_idx].descriptor_code = PT_UPPER_TABLE_DESCRIPTOR;
            tables_current += PAGE_SIZE;
        } else {
            return false;
        }
    }

    const auto l2_idx_start = (virt_addr >> L2_SHIFT) & PT_INDEX_MASK;
    const auto l2_idx_end   = ((virt_addr + size) >> L2_SHIFT) & PT_INDEX_MASK;
    auto* l2_tbl            = reinterpret_cast<u64*>(l2_tbl_ptr);
    // --- L2 table: we loop ---
    for (auto l2_idx = l2_idx_start; l2_idx < l2_idx_end; l2_idx++, phys_addr += L2_SIZE) {
        l2_tbl[l2_idx] = flags | phys_addr | PT_UPPER_BLOCK_DESCRIPTOR;
    }
    return true;
}

TableManager::TableManager(u8* current_embedded_table)
    : m_embedded_tables_current(current_embedded_table) {}
bool TableManager::map_region(uPtr virt_start, uPtr phys_start, uSize size, PageAttributes attrs,
                              MemAttributeIndex attr_idx) {
    // Check properly aligned to page.
    if (virt_start & PAGE_OFFSET_MASK || phys_start & PAGE_OFFSET_MASK || size & PAGE_OFFSET_MASK)
        return false;
    return map_upper(&__initial_pgtables_start, virt_start, phys_start, size,
                     static_cast<u64>(attrs) | (attr_idx << 2), L0);
}

bool TableManager::map_upper(u8* table, uPtr& virt_start, uPtr& phys_start, uSize& size, u64 flags,
                             TableLevel level) {
    VERIFY(level != L3);
    auto* tbl = reinterpret_cast<ARMv8MMU_UpperEntry*>(table);

    auto idx = (virt_start >> SHIFTS[level]) & PT_INDEX_MASK;

    for (; idx < PT_ENTRY_COUNT && size > 0; idx++) {
        auto entry_kind = tbl[idx].table.descriptor_code;
        u8* next_table  = nullptr;
        if (entry_kind == PT_INVALID_DESCRIPTOR) {
            // If block is appropriately sized + aligned, do a block entry in the table.
            if ((level == L1 || level == L2) && size >= SIZES[level] &&
                (virt_start & (SIZES[level] - 1)) == 0 && (phys_start & (SIZES[level] - 1)) == 0) {
                tbl[idx] = ARMv8MMU_UpperEntry::create_block_entry(phys_start, flags);
                virt_start += SIZES[level];
                phys_start += SIZES[level];
                size -= SIZES[level];
                continue;
            }

            // Allocate new table.
            next_table = allocate_table();
            tbl[idx] =
                ARMv8MMU_UpperEntry::create_table_entry(*mem::kernel_virt_to_phys(next_table));
            DBG::dbgln("New level {} table."_sv, static_cast<int>(level + 1));
        } else if (entry_kind == PT_UPPER_TABLE_DESCRIPTOR) {
            next_table = (u8*)mem::kernel_phys_to_virt({tbl[idx].table.table_page << PAGE_SHIFT});
        } else {
            // TODO: Spaces overlap!
            return false;
        }

        if (level == L2) {
            // Move to L3
            auto res = map_lower(next_table, virt_start, phys_start, size, flags);
            if (!res) return false;
        } else {
            auto res = map_upper(next_table, virt_start, phys_start, size, flags,
                                 static_cast<TableLevel>(level + 1));
            if (!res) return false;
        }
    }
    return true;
}
bool TableManager::map_lower(u8* table, uPtr& virt_start, uPtr& phys_start, uSize& size,
                             u64 flags) {
    auto* tbl = reinterpret_cast<ARMv8MMU_L3_Entry*>(table);
    auto idx  = (virt_start >> SHIFTS[L3]) & PT_INDEX_MASK;
    for (; idx < PT_ENTRY_COUNT && size > 0; idx++) {
        auto new_entry = ARMv8MMU_L3_Entry::create(phys_start, flags);
        if (tbl[idx].descriptor_code != PT_INVALID_DESCRIPTOR && tbl[idx] != new_entry) {
            return false;
        }
        tbl[idx] = new_entry;
        phys_start += SIZES[L3];
        virt_start += SIZES[L3];
        size -= SIZES[L3];
    }
    return true;
}
u8* TableManager::allocate_table() {
    if (m_embedded_tables_current < &__initial_pgtables_end) {
        auto* res = m_embedded_tables_current;
        m_embedded_tables_current += PAGE_SIZE;
        return res;
    } else {
        PANIC("I don't know how to get new pages!");
    }
}
