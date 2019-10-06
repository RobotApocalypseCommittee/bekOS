//
// Created by Joseph on 08/09/2019.
//

#ifndef BEKOS_PAGE_MAPPING_H
#define BEKOS_PAGE_MAPPING_H

#include <stdint.h>
#include <stddef.h>
#include "memory_manager.h"

// Granule size 4KB

struct ARMv8MMU_L3_Entry_4K {
    uint64_t descriptor_code: 2,
    lower_attrs: 10,
    address: 36,
    res0: 3,
    upper_attrs: 13;
} __attribute__ ((packed));

struct ARMv8MMU_L2_Entry_Table {
    uint64_t descriptor_code: 2,
    ignored_0: 10,
    table_page: 36,
    res0: 4,
    ignored: 7,
    attrs: 5;
} __attribute__ ((packed));
struct ARMv8MMU_L2_Entry_Block {
    uint64_t descriptor_code: 2,
    lower_attrs: 10,
    res0_0: 4,
    nT: 1,
    res0_1: 4,
    address: 27,
    res0_2: 4,
    upper_attrs: 12;
} __attribute__ ((packed));

class translation_table {
public:
    translation_table(memory_manager* manager);

    ~translation_table();

    bool map(uintptr_t vaddr, uintptr_t raddr);

private:
    memory_manager* manager;
    uintptr_t pages[256];
    size_t page_no;
    ARMv8MMU_L2_Entry_Table *table0;
    ARMv8MMU_L2_Entry_Table* map_table(ARMv8MMU_L2_Entry_Table* table, unsigned long shift, uintptr_t va);

};


#endif //BEKOS_PAGE_MAPPING_H
