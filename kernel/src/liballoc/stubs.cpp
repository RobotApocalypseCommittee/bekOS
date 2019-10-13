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

#include <stddef.h>
#include <memory_manager.h>
#include <utils.h>

extern memory_manager memoryManager;

extern "C" {

int liballoc_lock() {
    return 0;
}

int liballoc_unlock() {
    return 0;
}

void* liballoc_alloc(size_t pages) {
    return reinterpret_cast<void*>(phys_to_virt(memoryManager.reserve_region(pages, PAGE_KERNEL)));
}

int liballoc_free(void* ptr, size_t pages) {
    return !memoryManager.free_region(virt_to_phys(reinterpret_cast<unsigned long>(ptr)), pages);
}
}