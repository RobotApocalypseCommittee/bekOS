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

#include "mm/memory_manager.h"

#include "library/format.h"

namespace mem {

void bek_basic_format(bek::OutputStream& out, const AnnotatedRegion& region) {
    auto kind_str = (region.kind == AnnotatedRegion::Kind::Memory)     ? "Memory"_sv
                    : (region.kind == AnnotatedRegion::Kind::Reserved) ? "Reserved"_sv
                                                                       : "Unknown"_sv;
    bek::format_to(out, "{:XL} - {:XL} ({:XL}) {}"_sv, region.region.start.get(),
                   region.region.start.get() + region.region.size, region.region.size, kind_str);
}

bek::vector<AnnotatedRegion> process_memory_regions(
    const bek::vector<mem::PhysicalRegion>& mem_regions,
    const bek::vector<mem::PhysicalRegion>& reserved_regions) {
    // Here, reserved regions take priority over memory regions
    mem::PhysicalRegion remaining_region{{0}, 1ull << 48};
    bek::vector<AnnotatedRegion> regions;
    while (remaining_region.size) {
        AnnotatedRegion next_region{{{~0ull}, 0}, AnnotatedRegion::Kind::Unknown};
        for (auto& region : reserved_regions) {
            auto intersection = remaining_region.intersection(region);
            if (intersection.size && intersection.start < next_region.region.start) {
                next_region.region = intersection;
                next_region.kind   = AnnotatedRegion::Kind::Reserved;
            }
        }
        for (auto& region : mem_regions) {
            auto intersection = remaining_region.intersection(region);
            if (intersection.size && intersection.start < next_region.region.start) {
                // We go as far as possible.
                if (intersection.end() > next_region.region.start) {
                    // We overlap with a reserved region.
                    intersection.size = next_region.region.start.get() - intersection.start.get();
                }
                next_region.region = intersection;
                next_region.kind   = AnnotatedRegion::Kind::Memory;
            }
        }
        if (next_region.region.size) {
            // We have a good region.
            if (next_region.region.start != remaining_region.start) {
                // Need to add an unknown inbetween.
                regions.push_back({{remaining_region.start,
                                    next_region.region.start.get() - remaining_region.start.get()},
                                   AnnotatedRegion::Kind::Unknown});
            }
            regions.push_back(next_region);
            remaining_region.size -= next_region.region.end().get() - remaining_region.start.get();
            remaining_region.start = next_region.region.end();
        } else {
            regions.push_back({remaining_region, AnnotatedRegion::Kind::Unknown});
            remaining_region.start = remaining_region.end();
            remaining_region.size  = 0;
        }
    }
    return regions;
}

MemoryManager* memoryManager = nullptr;

MemoryManager& MemoryManager::the() {
    VERIFY(memoryManager);
    return *memoryManager;
}
void MemoryManager::initialise(const bek::vector<AnnotatedRegion>& regions,
                               u8* current_embedded_table) {
    memoryManager = new MemoryManager(current_embedded_table);
    for (auto& region : regions) {
        if (region.kind == AnnotatedRegion::Kind::Memory) {
            auto v_region = memoryManager->map_normal_memory(region.region);
            PageAllocator::the().register_new_region(v_region);
        }
    }
}

MemoryManager::MemoryManager(u8* current_embedded_table)
    : m_table_manager(current_embedded_table) {}
VirtualRegion MemoryManager::map_normal_memory(PhysicalRegion region) {
    auto v_ptr = VA_IDENT_OFFSET + region.start.get();
    auto r     = m_table_manager.map_region(v_ptr, region.start.get(), region.size, AttributesRWnE,
                                            NormalRAM);
    // TODO: Handle properly?
    VERIFY(r);
    return {{reinterpret_cast<u8*>(v_ptr)}, region.size};
}

DeviceArea MemoryManager::map_for_io(PhysicalRegion region) {
    auto r = m_table_manager.map_region(VA_IDENT_OFFSET + region.start.get(), region.start.get(),
                                        region.size, AttributesRWnE, MMIO);
    // TODO: Handle properly?
    VERIFY(r);
    return {region.start.get(), VA_IDENT_OFFSET + region.start.get(), region.size};
}

}  // namespace mem