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

#ifndef BEKOS_KMALLOC_H
#define BEKOS_KMALLOC_H

#include "library/types.h"

namespace mem {

struct AllocatedRegion {
    /// Pointer to start of allocated region.
    void* pointer;
    /// Size of region in bytes.
    uSize size;
};

inline constexpr uSize KERNEL_ALLOCATOR_DEFAULT_ALIGNMENT = 16;

void initialise_kmalloc();

/// Allocated region of at least size bytes, of alignment align.
/// \param size Size of region to request.
/// \param align Alignment of region; must be a power of two no greater than a page.
/// \return Allocated region, with a size which may be greater than requested. Region may be freed
/// by providing originally requested size.
AllocatedRegion allocate(uSize size, uSize align = KERNEL_ALLOCATOR_DEFAULT_ALIGNMENT);

/// Free a region allocated with allocate().
/// \param ptr Pointer returned by allocate().
/// \param size Size of allocation. Must be between requested size and size returned by allocate().
/// \param align Alignment of allocation. Must be same as that provided to allocate().
void free(void* ptr, uSize size, uSize align = KERNEL_ALLOCATOR_DEFAULT_ALIGNMENT);
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