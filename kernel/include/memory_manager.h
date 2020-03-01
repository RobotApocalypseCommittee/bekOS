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

#ifndef BEKOS_MEMORY_MANAGER_H
#define BEKOS_MEMORY_MANAGER_H

#include <stdint.h>
#include "memory_locations.h"

#define PAGE_NO (ADDRESSABLE_MEMORY/(4*1024))

enum page_states {
    PAGE_FREE = 0,
    PAGE_KERNEL = 1
};

class memory_manager {
public:
    uintptr_t reserve_region(int size, int reserver);
    bool free_region(uintptr_t location, int size);

    bool reserve_pages(uintptr_t location, int size, int reserver);

private:
    uint8_t page_list[PAGE_NO];
};

extern memory_manager memoryManager;

#endif //BEKOS_MEMORY_MANAGER_H
