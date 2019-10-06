//
// Created by Joseph on 22/09/2019.
//
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
