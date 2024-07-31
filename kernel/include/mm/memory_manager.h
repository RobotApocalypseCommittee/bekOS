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

#ifndef BEKOS_MEMORY_MANAGER_H
#define BEKOS_MEMORY_MANAGER_H

#include "arch/a64/translation_tables.h"
#include "areas.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "library/format_core.h"
#include "page_allocator.h"

namespace mem {

struct AnnotatedRegion {
    enum class Kind { Memory, Reserved, Unknown };
    mem::PhysicalRegion region;
    Kind kind;
};

void bek_basic_format(bek::OutputStream& out, const AnnotatedRegion& region);

bek::vector<AnnotatedRegion> process_memory_regions(
    const bek::vector<mem::PhysicalRegion>& mem_regions,
    const bek::vector<mem::PhysicalRegion>& reserved_regions);

class MemoryManager {
public:
    DeviceArea map_for_io(PhysicalRegion region);

    static MemoryManager& the();
    static void initialise(const bek::vector<AnnotatedRegion>& regions, u8* current_embedded_table);

private:
    explicit MemoryManager(u8* current_embedded_table);
    VirtualRegion map_normal_memory(PhysicalRegion region);

private:
    TableManager m_table_manager;
};

}  // namespace mem

#endif  // BEKOS_MEMORY_MANAGER_H
