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

#ifndef BEKOS_KMALLOC_H
#define BEKOS_KMALLOC_H

#include "bek/types.h"
#include "bek/utility.h"

namespace mem {

struct AllocatedRegion {
    /// Pointer to start of allocated region.
    void* pointer;
    /// Size of region in bytes.
    uSize size;
};

void initialise_kmalloc();



bek::pair<uSize, uSize> get_kmalloc_usage();
}  // namespace mem

namespace std {
enum class align_val_t : uSize {};  // NOLINT(*-dcl58-cpp)

struct nothrow_t {
    explicit nothrow_t() = default;
};                                    // NOLINT(*-dcl58-cpp)
extern const std::nothrow_t nothrow;  // NOLINT(*-dcl58-cpp)
}  // namespace std

// Placement Operators
inline void* operator new(uSize, void* ptr) noexcept { return ptr; }
inline void* operator new[](uSize, void* ptr) noexcept { return ptr; }
// void operator delete  (void* ptr, void* place ) noexcept;
// void operator delete[](void* ptr, void* place ) noexcept;

inline void* kmalloc(uSize sz) { return mem::allocate(sz).pointer; }
inline void* kmalloc(uSize sz, uSize align) { return mem::allocate(sz, align).pointer; }

inline void kfree(void* ptr, uSize sz) { return mem::free(ptr, sz); }
inline void kfree(void* ptr, uSize sz, uSize align) { return mem::free(ptr, sz, align); }

#endif  // BEKOS_KMALLOC_H