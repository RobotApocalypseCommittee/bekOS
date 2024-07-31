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

#ifndef BEKOS_TRANSLATION_TABLES_H
#define BEKOS_TRANSLATION_TABLES_H

#include <arch/a64/memory_constants.h>
#include <arch/a64/sysreg_constants.h>

#include "bek/types.h"
#include "c_string.h"
#include "library/optional.h"
#include "mm/addresses.h"

enum MemAttributeIndex {
    NormalRAM = MAIR_NORMAL_NC_INDEX,
    MMIO      = MAIR_DEVICE_nGnRnE_INDEX,
};

enum TableLevel { L0 = 0, L1 = 1, L2 = 2, L3 = 3 };

#define BIT(nr) (1UL << (nr))
enum PageAttributes : u64 {
    UnprivilegedExecuteNever = BIT(54),
    PrivilegedExecuteNever   = BIT(53),
    Contiguous               = BIT(52),
    DirtyBitModifier         = BIT(51),
    GP                       = BIT(50),
    /// Not 4KB
    nT        = BIT(16),
    nG        = BIT(11),
    AF        = BIT(10),
    ReadOnly  = BIT(7),
    EL0Access = BIT(6)
};

constexpr inline PageAttributes AttributesRWnE =
    static_cast<PageAttributes>(PrivilegedExecuteNever | AF);
constexpr inline PageAttributes AttributesRnWE = static_cast<PageAttributes>(ReadOnly | AF);
constexpr inline PageAttributes AttributedRnWnE =
    static_cast<PageAttributes>(ReadOnly | PrivilegedExecuteNever | AF);

/// A very crude mapping function, designed for early boot (no constant references). virt_addr,
/// phys_addr and size must be 2MB aligned. tables_* must be physical pointers, accessible from the
/// CPU also.
bool crude_map_region(uPtr virt_addr, uPtr phys_addr, uSize size, u64 flags, uPtr tables_start,
                      uPtr& tables_current, uPtr tables_end);

class TableManager {
public:
    explicit TableManager(u8* current_embedded_table);
    bool map_region(uPtr virt_start, uPtr phys_start, uSize size, PageAttributes attrs,
                    MemAttributeIndex attr_idx);

private:
    bool map_upper(u8* table, uPtr& virt_start, uPtr& phys_start, uSize& size, u64 flags,
                   TableLevel level);
    bool map_lower(u8* table, uPtr& virt_start, uPtr& phys_start, uSize& size, u64 flags);
    u8* allocate_table();

    u8* m_embedded_tables_current;
};

#endif  // BEKOS_TRANSLATION_TABLES_H
