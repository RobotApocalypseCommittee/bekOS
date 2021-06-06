//
// Created by joebe on 16/05/2021.
//

#ifndef BEKOS_PAGE_TABLE_H
#define BEKOS_PAGE_TABLE_H

#include "library/types.h"

struct mem_region {
    uPtr start;
    uPtr length;
};

enum EntryType {
    Invalid,
    Block,
    Table
};

enum TableLevel {
    L0 = 0,
    L1 = 1,
    L2 = 2,
    L3 = 3
};

#define BIT(nr) (1UL << (nr))
enum PageAttributes: u64 {
    UnprivilegedExecuteNever = BIT(54),
    PrivilegedExecuteNever = BIT(53),
    Contiguous = BIT(52),
    DirtyBitModifier = BIT(51),
    GP = BIT(50),
    /// Not 4KB
    nT = BIT(16),
    nG = BIT(11),
    AF = BIT(10),
    ReadOnly = BIT(7),
    EL0Access = BIT(6)
};

struct ARMv8MMU_L3_Entry {
    u64 descriptor_code: 2,
        lower_attrs: 10,
        address: 36,
        res0: 3,
        upper_attrs: 13;
} __attribute__ ((packed));

ARMv8MMU_L3_Entry create_L3_block_entry(uPtr bus_address, u64 flags);

struct ARMv8MMU_Upper_Entry_Table {
    u64 descriptor_code: 2,
        ignored_0: 10,
        table_page: 36,
        res0: 4,
        ignored: 7,
        attrs: 5;
} __attribute__ ((packed));



struct ARMv8MMU_Upper_Entry_Block {
    u64 descriptor_code: 2,
        lower_attrs: 10,
        res0_0: 4,
        nT: 1,
        res0_1: 4,
        address: 27,
        res0_2: 4,
        upper_attrs: 12;
} __attribute__ ((packed));

union ARMv8MMU_Upper_Entry {
    ARMv8MMU_Upper_Entry_Table table;
    ARMv8MMU_Upper_Entry_Block block;
};

ARMv8MMU_Upper_Entry create_upper_block_entry(uPtr bus_address, TableLevel level, u64 flags);
ARMv8MMU_Upper_Entry create_upper_table_entry(uPtr next_table_address, TableLevel level);

EntryType getUpperEntryType(ARMv8MMU_Upper_Entry e);
EntryType getLowerEntryType(ARMv8MMU_L3_Entry e);

class basic_translation_table {
public:
    explicit basic_translation_table(mem_region usable_memory);

    bool map_region(uPtr virtual_address, uPtr bus_address, uSize size, PageAttributes attributes, u8 memattridx);
    //uPtr traverse(uPtr virtual_address) const;

private:

    uPtr allocate_table();

    static bool map_lower_region(ARMv8MMU_L3_Entry* table, uPtr& virtual_address, uPtr& bus_address, uSize& size, u64 page_flags);

    bool map_upper_region(ARMv8MMU_Upper_Entry* table, TableLevel level, uPtr& virtual_address, uPtr& bus_address, uSize& size, u64 page_flags);

    uPtr region_end;
    uPtr allocated_end;
    ARMv8MMU_Upper_Entry* level0table;
};


#endif  // BEKOS_PAGE_TABLE_H
