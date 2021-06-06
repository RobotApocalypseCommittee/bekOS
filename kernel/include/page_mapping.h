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

#ifndef BEKOS_PAGE_MAPPING_H
#define BEKOS_PAGE_MAPPING_H

#include "memory_manager.h"
#include "library/vector.h"
#include "mm/hw_ptr.h"

// Granule size 4KB

struct ARMv8MMU_L3_Entry_4K {
    u64 descriptor_code: 2,
    lower_attrs: 10,
    address: 36,
    res0: 3,
    upper_attrs: 13;
} __attribute__ ((packed));

struct ARMv8MMU_L2_Entry_Table {
    u64 descriptor_code: 2,
    ignored_0: 10,
    table_page: 36,
    res0: 4,
    ignored: 7,
    attrs: 5;
} __attribute__ ((packed));
struct ARMv8MMU_L2_Entry_Block {
    u64 descriptor_code: 2,
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
    /// This class works in both mapped and unmapped space - cannot contain any permanent pointer.
    translation_table(hw_ptr<memory_manager> mem_manager);
    ~translation_table();

    bool map_range(uPtr virtual_address, uPtr bus_address, uSize len);
    uPtr traverse(uPtr virtual_address) const;
private:
    hw_ptr<ARMv8MMU_L2_Entry_Table> level0table;
    // TODO: Figure out how to keep track of allocated pages for tables
    hw_ptr<memory_manager> allocator;
};





#endif //BEKOS_PAGE_MAPPING_H
