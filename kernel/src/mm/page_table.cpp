//
// Created by joebe on 16/05/2021.
//
#include "mm/page_table.h"

#include <kstring.h>
#include <library/assert.h>
#include <mm.h>
#include <printf.h>

unsigned shifts[] = {LEVEL_0_SHIFT, LEVEL_1_SHIFT, LEVEL_2_SHIFT, LEVEL_3_SHIFT};
#define PAGE_ATTRIBUTES ()
basic_translation_table::basic_translation_table(mem_region usable_memory): region_end(usable_memory.start + usable_memory.length), level0table(reinterpret_cast<ARMv8MMU_Upper_Entry*>(usable_memory.start)), allocated_end(usable_memory.start) {
    assert(usable_memory.length >= PAGE_SIZE);
    allocated_end += PAGE_SIZE;
    memset(level0table, 0, PAGE_SIZE);
}

bool basic_translation_table::map_region(uPtr virtual_address, uPtr bus_address, uSize size,
                                         PageAttributes attributes, u8 memattridx) {
    assert((size % PAGE_SIZE) == 0);
    assert((virtual_address % PAGE_SIZE) == 0);
    assert((bus_address % PAGE_SIZE) == 0);
    u64 flags = static_cast<u64>(attributes) | (memattridx & 0b111) << 2;
    if (map_upper_region(level0table, L0, virtual_address, bus_address, size, flags)) {
        assert(size == 0);
        return true;
    } else {
        return false;
    }
}

bool basic_translation_table::map_upper_region(ARMv8MMU_Upper_Entry* table, TableLevel level,
                                               uPtr& virtual_address, uPtr& bus_address,
                                               uSize& size, u64 page_flags) {
    assert(level != L3); // Upper level
    unsigned start_idx = (virtual_address >> shifts[level]) % PAGE_ENTRY_COUNT;
    for (unsigned i = start_idx; i < PAGE_ENTRY_COUNT && size > 0; i++) {
        // Each iteration MUST map one block's worth only.
        auto entryType = getUpperEntryType(table[i]);
        uPtr next_table;
        if (entryType == Invalid) {
            // We need to allocate a new table (or L2 block)
            // If L2, consider this possibility:
            if (level == L2 && size >= LEVEL_2_SIZE) {
                // Direct to a block on level 2
                table[i] = create_upper_block_entry(bus_address, L2, page_flags);
                //printf("[MM] Map 2MB: va=%lX, ba=%lX\n", virtual_address, bus_address);
                virtual_address += LEVEL_2_SIZE;
                bus_address += LEVEL_2_SIZE;
                size -= LEVEL_2_SIZE;
                continue;
            } else {
                // Create a new table
                next_table = allocate_table();
                table[i] = create_upper_table_entry(next_table, level);
            }
        } else if (entryType == Block) {
            // TODO: Being asked to overwrite
            //printf("[MM] Being asked to overwrite? va=%lX, ba=%lX\n", virtual_address, bus_address);
            return false;
        } else {
            // Entry type is table -> follow
            next_table = table[i].table.table_page * PAGE_SIZE;
        }
        // Move on to lower
        bool res;
        if (level == L2) {
            res = map_lower_region(reinterpret_cast<ARMv8MMU_L3_Entry*>(next_table),
                             virtual_address, bus_address, size, page_flags);
        } else {
            res = map_upper_region(reinterpret_cast<ARMv8MMU_Upper_Entry*>(next_table),
                             static_cast<TableLevel>(level + 1), virtual_address, bus_address, size, page_flags);
        }
        if (!res) {return false;}
    }
    return true;
}
bool basic_translation_table::map_lower_region(ARMv8MMU_L3_Entry* table, uPtr& virtual_address,
                                               uPtr& bus_address, uSize& size, u64 page_flags) {
    unsigned start_idx = (virtual_address >> LEVEL_3_SHIFT) % PAGE_ENTRY_COUNT;
    for (unsigned i = start_idx; i < PAGE_ENTRY_COUNT && size > 0; i++) {
        // Map my pages
        table[i] = create_L3_block_entry(bus_address, page_flags);
        //printf("[MM] Map 4KB: va=%lX, ba=%lX\n", virtual_address, bus_address);
        bus_address += LEVEL_3_SIZE;
        virtual_address += LEVEL_3_SIZE;
        size -= LEVEL_3_SIZE;
    }
    return true;
}
uPtr basic_translation_table::allocate_table() {
    auto page = allocated_end;
    allocated_end += PAGE_SIZE;
    assert(allocated_end <= region_end);
    memset(reinterpret_cast<void*>(page), 0, PAGE_SIZE);
    return page;

}
EntryType getUpperEntryType(ARMv8MMU_Upper_Entry e) {
    if (e.block.descriptor_code == 0b01) {
        return Block;
    } else if (e.block.descriptor_code == 0b11) {
        return Table;
    } else {
        return Invalid;
    }
}
EntryType getLowerEntryType(ARMv8MMU_L3_Entry e) {
    if (e.descriptor_code == 0b11) {
        return Block;
    } else {
        return Invalid;
    }
}
ARMv8MMU_L3_Entry create_L3_block_entry(uPtr bus_address, u64 flags) {
    union {
        u64 raw;
        ARMv8MMU_L3_Entry entry;
    } n;
    n.raw = flags;
    n.entry.address = (bus_address >> PAGE_SHIFT);
    n.entry.descriptor_code = 0b11;
    return n.entry;
}
ARMv8MMU_Upper_Entry create_upper_block_entry(uPtr bus_address, TableLevel level,
                                              u64 flags) {

    union {
        u64 raw;
        ARMv8MMU_Upper_Entry_Block entry;
        ARMv8MMU_Upper_Entry final;
    } n;
    n.raw = flags;
    assert(level == L1 || level == L2);
    if (level == L1) {
        // Target a GB block
        assert((bus_address % LEVEL_1_SIZE) == 0);
        n.entry.address = (bus_address >> 21);
    } else {
        // Target a 2MB block
        assert((bus_address % LEVEL_2_SIZE) == 0);
        n.entry.address = bus_address >> 21;
    }
    n.entry.descriptor_code = 0b01;

    return n.final;
}
ARMv8MMU_Upper_Entry create_upper_table_entry(uPtr next_table_address, TableLevel level) {
    union {
        u64 raw;
        ARMv8MMU_Upper_Entry_Table entry;
        ARMv8MMU_Upper_Entry final;
    } n;
    // We dont bother with no flags
    n.raw = 0;
    assert((next_table_address & (PAGE_SIZE - 1)) == 0);
    n.entry.table_page = next_table_address >> PAGE_SHIFT;
    n.entry.descriptor_code = 0b11;
    return n.final;
}
