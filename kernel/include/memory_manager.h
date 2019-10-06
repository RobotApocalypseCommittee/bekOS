//
// Created by Joseph on 22/09/2019.
//

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

#endif //BEKOS_MEMORY_MANAGER_H
