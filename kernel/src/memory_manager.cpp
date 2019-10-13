/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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

#include "memory_manager.h"

uintptr_t memory_manager::reserve_region(int size, int reserver) {
    int i = 0;
    int block_count = 0;
    int block_start = 0;
    while (i < PAGE_NO) {
        if (page_list[i] == PAGE_FREE) {
            if (block_count == 0) {
                block_start = i;
            }
            block_count++;
        } else {
            block_count = 0;
        }
        if (block_count == size) {
            break;
        }
        i++;
    }
    if (i == PAGE_NO) {
        // TODO: Disaster - error run out(store pages in swap?)
    }
    for (i = block_start; i < (block_start + size); i++) {
        page_list[i] = reserver;
    }
    // Page is 4096 bytes
    return block_start * 4096;
}

bool memory_manager::free_region(uintptr_t location, int size) {
    location = location/4096;
    if (page_list[location] != PAGE_FREE) {
        for (unsigned long i = location; i < (location + size); i++) {
            page_list[i] = PAGE_FREE;
        }
        return true;
    } else {
        // TODO: Why free free blocks?
        return false;
    }
}

bool memory_manager::reserve_pages(uintptr_t location, int size, int reserver) {
    location = location/4096;
    for (unsigned long i = location; i < (location+size); i++) {
        page_list[i] = reserver;
    }
    return true;

}
