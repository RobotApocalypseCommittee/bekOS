// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <bek/allocations.h>

#include "core/syscall.h"

constexpr inline uSize MIN_BLOCK_SIZE = 8;

constexpr bool is_aligned(uPtr ptr, uSize alignment) { return ptr % alignment == 0; }

constexpr bool is_power_of_two(uSize n) { return n != 0 && (n & (n - 1ul)) == 0; }

struct BlockHeader {
    BlockHeader* next;
    uSize size;

    void* data() { return (reinterpret_cast<char*>(this) + sizeof(BlockHeader)); }

    void* end() { return (reinterpret_cast<char*>(this) + size); }

    void split_block(uSize target_size) {
        VERIFY(can_be_split(target_size));
        auto second_size = data_size() - target_size;
        size = sizeof(BlockHeader) + target_size;
        next = new (end()) BlockHeader(next, second_size);
    }

    constexpr bool can_be_split(uSize target_size) {
        // Split requires (a) size >= 2*BlockHeader + target_size + min_block_size
        return size >= sizeof(BlockHeader) * 2 + target_size + MIN_BLOCK_SIZE;
    }

    constexpr uSize data_size() const { return size - sizeof(BlockHeader); }
};

struct HugeBlockHeader {
    HugeBlockHeader* next;
    uSize size;

    void* data() { return (reinterpret_cast<char*>(this) + sizeof(HugeBlockHeader)); }
};

constexpr inline uSize PAGE_SIZE = 4096;
constexpr inline uSize SIZE_FOR_SEPARATE_LARGE_BLOCK = PAGE_SIZE;
constexpr inline uSize DEFAULT_LARGE_BLOCK_SIZE = PAGE_SIZE * 16;

HugeBlockHeader* g_huge_block_head = nullptr;
BlockHeader* g_free_blocks = nullptr;

// Debug Checks
void* g_heap_start = nullptr;
void* g_heap_end = nullptr;
void update_heap_range_tracker(void* retptr) {
    if (!g_heap_start || retptr < g_heap_start) {
        g_heap_start = retptr;
    }

    if (!g_heap_end || retptr > g_heap_end) {
        g_heap_end = retptr;
    }
}

// We will not free any (standard-ly allocated) blocks to the kernel -> one unified free list.
/// Inserts a block into the list, maintaining sort order.
/// \param block Block to insert. Must not already be in list.
/// \return the precursor block; nullptr if new block is at front.
BlockHeader* insert_block(BlockHeader* block) {
    BlockHeader* next_block = g_free_blocks;
    BlockHeader* last_block = nullptr;
    while (next_block) {
        if (next_block > block) {
            break;
        }

        last_block = next_block;
        next_block = next_block->next;
    }

    block->next = next_block;
    if (last_block) {
        last_block->next = block;
    } else {
        g_free_blocks = block;
    }
    return last_block;
}

mem::AllocatedRegion mem::allocate(uSize size, uSize align) {
    if (size == 0) {
        return {nullptr, 0};
    }
    VERIFY(align <= PAGE_SIZE);

    // all structs are 8-byte-aligned - so make all regions thus also.
    size = bek::align_up(size, 8ul);
    if (size >= SIZE_FOR_SEPARATE_LARGE_BLOCK) {
        uSize hb_size = bek::align_up(size + sizeof(HugeBlockHeader), PAGE_SIZE);
        auto res = core::syscall::allocate(sc::INVALID_ADDRESS_VAL, hb_size, sc::AllocateFlags::None);
        if (res.has_error()) return {nullptr, 0};
        auto* huge_block = new (reinterpret_cast<void*>(res.value())) HugeBlockHeader(g_huge_block_head, hb_size);
        g_huge_block_head = huge_block;
        update_heap_range_tracker(huge_block->data());
        return {huge_block->data(), hb_size - sizeof(HugeBlockHeader)};
    } else {
        BlockHeader* best_block = nullptr;
        BlockHeader* best_block_prev = nullptr;

        BlockHeader* next_block = g_free_blocks;
        BlockHeader* last_block = nullptr;
        while (next_block) {
            if (next_block->data_size() >= size) {
                // Suitable block
                if (!best_block || best_block->data_size() > next_block->data_size()) {
                    best_block = next_block;
                    best_block_prev = last_block;

                    if (!best_block->can_be_split(size)) {
                        // Close fitting block -> not gonna get closer than this.
                        break;
                    }
                }
            }

            last_block = next_block;
            next_block = next_block->next;
        }

        if (!best_block) {
            // Need new set of blocks.
            auto res =
                core::syscall::allocate(sc::INVALID_ADDRESS_VAL, DEFAULT_LARGE_BLOCK_SIZE, sc::AllocateFlags::None);
            if (res.has_error()) return {nullptr, 0};
            best_block = new (reinterpret_cast<void*>(res.value())) BlockHeader(nullptr, DEFAULT_LARGE_BLOCK_SIZE);
            best_block_prev = insert_block(best_block);
        }

        // Do we bother splitting the block?
        if (best_block->can_be_split(size)) {
            best_block->split_block(size);
        }

        auto* following_block = best_block->next;
        best_block->next = nullptr;
        if (best_block_prev) {
            best_block_prev->next = following_block;
        } else {
            g_free_blocks = following_block;
        }

        update_heap_range_tracker(best_block->data());
        return {best_block->data(), best_block->data_size()};
    }
}

bool try_free_huge(void* ptr) {
    HugeBlockHeader* prev_hb = nullptr;
    HugeBlockHeader* next_hb = g_huge_block_head;
    while (next_hb) {
        if (ptr == next_hb->data()) {
            // Free this block.
            if (prev_hb) {
                prev_hb->next = next_hb->next;
            } else {
                g_huge_block_head = next_hb->next;
            }
            core::syscall::deallocate(reinterpret_cast<uPtr>(next_hb), next_hb->size);
            return true;
        }
        prev_hb = next_hb;
        next_hb = prev_hb->next;
    }
    return false;
}

void free_small(void* ptr) {
    // Not a huge block.
    auto* header = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(ptr) - sizeof(BlockHeader));
    // Small sanity check.
    VERIFY(8 <= header->data_size() && header->data_size() < SIZE_FOR_SEPARATE_LARGE_BLOCK);

    // Insert it into list.
    BlockHeader* previous = insert_block(header);
    // Now, coallesce if possible.
    if (header->next == header->end()) {
        header->size = header->size + header->next->size;
        header->next = header->next->next;
    }

    if (previous && header == previous->end()) {
        previous->size = previous->size + header->size;
        previous->next = header->next;
    }
}

void free(void* ptr) {
    if (ptr == nullptr) return;

    VERIFY(g_heap_start <= ptr && ptr <= g_heap_end);

    if (try_free_huge(ptr)) return;
    free_small(ptr);
}

uSize get_size_of_allocation(void* ptr) {
    if (ptr == nullptr) return 0;
    VERIFY(g_heap_start <= ptr && ptr <= g_heap_end);
    // Try huge block
    {
        HugeBlockHeader* prev_hb = nullptr;
        HugeBlockHeader* next_hb = g_huge_block_head;
        while (next_hb) {
            if (ptr == next_hb->data()) {
                return next_hb->size - sizeof(HugeBlockHeader);
            }
            prev_hb = next_hb;
            next_hb = prev_hb->next;
        }
    }
    // Is a small block
    auto* header = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(ptr) - sizeof(BlockHeader));
    // Small sanity check.
    VERIFY(8 <= header->data_size() && header->data_size() < SIZE_FOR_SEPARATE_LARGE_BLOCK);
    return header->data_size();
}

void* realloc(void* ptr, uSize size) {
    if (ptr == nullptr) return mem::allocate(size).pointer;
    if (auto data_size = get_size_of_allocation(ptr); size > data_size) {
        auto new_alloc = mem::allocate(size);
        if (new_alloc.pointer) {
            bek::memcopy(new_alloc.pointer, ptr, data_size);
        }
        free(ptr);
        return new_alloc.pointer;
    } else {
        return ptr;
    }
}

void mem::free(void* ptr, uSize size, uSize align) {
    (void)align;
    (void)size;
    ::free(ptr);
}

void* operator new(uSize count) { return mem::allocate(count).pointer; }
void* operator new[](uSize count) { return mem::allocate(count).pointer; }
void* operator new(uSize count, std::align_val_t al) { return mem::allocate(count, static_cast<uSize>(al)).pointer; }
void* operator new[](uSize count, std::align_val_t al) { return mem::allocate(count, static_cast<uSize>(al)).pointer; }
void operator delete(void* ptr, uSize sz) noexcept { mem::free(ptr, sz); }
void operator delete[](void* ptr, uSize sz) noexcept { mem::free(ptr, sz); }
void operator delete(void* ptr, uSize sz, std::align_val_t al) noexcept { mem::free(ptr, sz, static_cast<uSize>(al)); }
void operator delete[](void* ptr, uSize sz, std::align_val_t al) noexcept {
    mem::free(ptr, sz, static_cast<uSize>(al));
}
