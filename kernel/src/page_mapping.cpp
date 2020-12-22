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

#include <mm.h>
#include <utils.h>
#include "page_mapping.h"

bool translation_table::map(uPtr vaddr, uPtr raddr) {
    // Map lvl3
    ARMv8MMU_L2_Entry_Table* lvl1table = map_table(table0, LEVEL_0_SHIFT, vaddr);

    ARMv8MMU_L2_Entry_Table* lvl2table = map_table(reinterpret_cast<ARMv8MMU_L2_Entry_Table*>(lvl1table), LEVEL_1_SHIFT, vaddr);

    auto* lvl3table = reinterpret_cast<ARMv8MMU_L3_Entry_4K*>(map_table(
            reinterpret_cast<ARMv8MMU_L2_Entry_Table*>(lvl2table), LEVEL_2_SHIFT, vaddr));

    // Mapping the page entry

    unsigned long lvl3index = (vaddr >> LEVEL_3_SHIFT) & (PAGE_ENTRY_COUNT - 1);
    // TODO: Headerise and make sense
    ARMv8MMU_L3_Entry_4K entry4K;
    entry4K.upper_attrs = 0;
    entry4K.lower_attrs = 0b0100010001;
    entry4K.res0 = 0;
    entry4K.descriptor_code = 0b11;
    entry4K.address = raddr>>PAGE_SHIFT;
    lvl3table[lvl3index] = entry4K;
    return true;
}

ARMv8MMU_L2_Entry_Table* translation_table::map_table(ARMv8MMU_L2_Entry_Table* table, unsigned long shift, uPtr va) {
    // The index of the table we are checking
    unsigned long table_index = (va >> shift) & (PAGE_ENTRY_COUNT - 1);
    // Check whether this table is already mapped(to another table)
    if (table[table_index].descriptor_code == 0b11) {
        return reinterpret_cast<ARMv8MMU_L2_Entry_Table*>(phys_to_virt(table[table_index].table_page * PAGE_SIZE));
    } else if (table[table_index].descriptor_code == 0b00) {
        // Is invalid(not existing yet)
        uPtr new_page = manager->reserve_region(1, PAGE_KERNEL);
        memzero(phys_to_virt(new_page), PAGE_SIZE);
        pages.push_back(new_page);
        ARMv8MMU_L2_Entry_Table entry;
        entry.descriptor_code = 0b11;
        entry.table_page = new_page/4096;
        entry.attrs = 0;
        entry.ignored_0 = 0;
        entry.ignored = 0;
        entry.res0 = 0;
        table[table_index] = entry;
        return reinterpret_cast<ARMv8MMU_L2_Entry_Table*>(phys_to_virt(new_page));
    } else {
        // ??
        return nullptr;
    }
}

translation_table::~translation_table() {
    for (uSize i = 0; i < pages.size(); i++) {
        manager->free_region(pages[i], 1);
    }

}

translation_table::translation_table(memory_manager* manager) : manager(manager) {
    // Is invalid(not existing yet)
    uPtr new_page = manager->reserve_region(1, PAGE_KERNEL);
    memzero(phys_to_virt(new_page), PAGE_SIZE);
    pages.push_back(new_page);
    table0 = reinterpret_cast<ARMv8MMU_L2_Entry_Table*>(phys_to_virt(new_page));
}

translation_table::translation_table(): translation_table(&memoryManager){}

u64 translation_table::get_table_address() {
    return virt_to_phys(reinterpret_cast<unsigned long>(table0));
}
