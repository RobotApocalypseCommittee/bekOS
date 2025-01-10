// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
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

#include "mm/kmalloc.h"

#include <library/bitset.h>

#include "arch/a64/memory_constants.h"
#include "library/debug.h"
#include "library/intrusive_list.h"
#include "mm/page_allocator.h"

constexpr bool is_aligned(uPtr ptr, uSize alignment) { return ptr % alignment == 0; }

using DBG = DebugScope<"kmalloc", DebugLevel::WARN>;

class BitmapAllocator {
public:
    static constexpr uSize chunk_size = 128;
    BitmapAllocator(char* data, uSize size)
        : m_data{data},
          m_chunk_count{calculate_chunk_count(size)},
          m_bitmap((data + m_chunk_count * chunk_size), m_chunk_count) {
        // Sanity check - bytes taken by chunks + bytes taken by bitmap < size.
        VERIFY(m_chunk_count * chunk_size + bek::ceil_div(m_chunk_count, 8ul) <= size);
    }

    void* allocate(uSize size, uSize alignment = 1) {
        auto chunks_needed = bek::ceil_div(size, chunk_size);

        // This is strictly inaccurate for most alignment values.
        // But guaranteed to be valid if alignment and chunk_size are powers of 2.
        auto chunk_alignment = bek::ceil_div(alignment, chunk_size);
        VERIFY((chunk_alignment * chunk_size) % alignment == 0);

        // Ensure aligned to chunk size.
        VERIFY(reinterpret_cast<uPtr>(m_data) % chunk_size == 0);
        auto align_offset = (reinterpret_cast<uPtr>(m_data) / chunk_size) % chunk_alignment;

        auto result =
            m_bitmap.find_first_fit(chunks_needed, false, 0, chunk_alignment, align_offset);
        if (result.is_invalid()) return nullptr;

        // Mark as used.
        m_bitmap.set_range(result.index, chunks_needed, true);
        m_allocated_chunks += chunks_needed;
        return m_data + result.index * chunk_size;
    }

    void free(void* ptr, uSize size) {
        VERIFY(m_data <= ptr && ptr < (m_data + chunk_size * m_chunk_count));
        auto chunk_index    = (reinterpret_cast<char*>(ptr) - m_data) / chunk_size;
        auto chunks_to_free = bek::ceil_div(size, chunk_size);
        // TODO: Verify flip?
        m_bitmap.set_range(chunk_index, chunks_to_free, false);
        m_allocated_chunks -= chunks_to_free;
    }

    [[nodiscard]] uSize total_bytes() const { return m_chunk_count * chunk_size; }
    [[nodiscard]] uSize free_bytes() const {
        return (m_chunk_count - m_allocated_chunks) * chunk_size;
    }

private:
    static constexpr uSize calculate_chunk_count(uSize size) {
        // Each chunk requires chunk_size bytes, plus one bitmap bit,
        // so 8(chunk_size) + 1 bits.
        return (size * 8) / (chunk_size * 8 + 1);
    }

    char* m_data;
    uSize m_chunk_count;
    uSize m_allocated_chunks{0};
    bek::bitset_view m_bitmap;
};

struct SlabBlock {
    /// Must be placement-new-ed into start of block.
    SlabBlock(uSize obj_size, uSize block_size)
        : object_size{obj_size},
          allocated_objects{0},
          total_objects{static_cast<int>((block_size - sizeof(SlabBlock)) / obj_size)} {
        char* block_array = data_begin();

        // This will initialise in 'reverse-order' - last object will be first.
        for (int i = 0; i < total_objects; i++) {
            auto* freelist_element =
                new (&block_array[i * obj_size]) FreeListElement{freelist_head};
            freelist_head = freelist_element;
        }
    }

    void* allocate() {
        if (!freelist_head) return nullptr;
        ++allocated_objects;
        void* ret     = freelist_head;
        freelist_head = freelist_head->next;
        return ret;
    }

    void deallocate(void* ptr) {
        VERIFY(ptr >= data_begin() && ptr < data_end());
        VERIFY(is_aligned(reinterpret_cast<uPtr>(ptr), object_size));
        --allocated_objects;
        freelist_head = new (ptr) FreeListElement{freelist_head};
    }

    char* data_begin() {
        auto start_index = bek::ceil_div(sizeof(SlabBlock), object_size);
        return &reinterpret_cast<char*>(this)[start_index * object_size];
    }

    char* data_end() { return &data_begin()[total_objects * object_size]; }

    [[nodiscard]] uSize total_bytes() const { return total_objects * object_size; }
    [[nodiscard]] uSize free_bytes() const {
        return (total_objects - allocated_objects) * object_size;
    }

    // Only need forward traversal. Simpler.
    struct FreeListElement {
        FreeListElement* next;
    };
    uSize object_size;
    FreeListElement* freelist_head{nullptr};

    bek::IntrusiveListNode<SlabBlock> node;
    using List = bek::IntrusiveList<SlabBlock, &SlabBlock::node>;

    int allocated_objects;
    int total_objects;
};

class SlabAllocator {
    static constexpr uSize default_block_size(uSize obj_size) {
        if (obj_size <= 256) return 4096;
        return (obj_size / 256) * 4096;
    }

public:
    SlabAllocator(uSize object_size, uSize block_size)
        : m_obj_size{object_size}, m_blk_size{block_size} {
        VERIFY(object_size >= 32);
    }
    explicit SlabAllocator(uSize object_size)
        : SlabAllocator(object_size, default_block_size(object_size)) {}

    [[nodiscard]] constexpr uSize object_size() const { return m_obj_size; }

    void* allocate() {
        if (m_free_blocks.empty()) {
            auto region = mem::allocate(m_blk_size, m_blk_size);
            if (!region.pointer) {
                DBG::errln("SlabBlock({}) could not allocate another block of size {}."_sv,
                           m_obj_size, m_blk_size);
            }
            auto* block = new (region.pointer) SlabBlock(m_obj_size, m_blk_size);
            m_free_blocks.append(*block);
        }

        SlabBlock& block = m_free_blocks.front();
        void* allocation = block.allocate();
        // If block now full, place in other list.
        if (block.allocated_objects == block.total_objects) {
            m_free_blocks.remove(block);
            m_full_blocks.append(block);
        }
        return allocation;
    }

    void free(void* ptr) {
        // Get block base:
        auto* block =
            reinterpret_cast<SlabBlock*>((reinterpret_cast<uPtr>(ptr) / m_blk_size) * m_blk_size);
        if (block->allocated_objects == block->total_objects) {
            // Block is full
            m_full_blocks.remove(*block);
            m_free_blocks.append(*block);
        }
        block->deallocate(ptr);
    }

    [[nodiscard]] uSize total_bytes() const {
        return (m_free_blocks.size() + m_full_blocks.size()) * m_blk_size;
    }
    [[nodiscard]] uSize free_bytes() const {
        uSize free = 0;
        for (auto& item : m_free_blocks) {
            free += item.free_bytes();
        }
        return free;
    }

private:
    SlabBlock::List m_free_blocks;
    SlabBlock::List m_full_blocks;
    uSize m_obj_size;
    uSize m_blk_size;
};

struct KernelAllocator {
    KernelAllocator(char* initial_start, uSize initial_size)
        : m_bitmap_allocator(initial_start, initial_size) {}

    mem::AllocatedRegion allocate(uSize size, uSize align) {
        if (size >= 64 * KiB) {
            VERIFY(align <= PAGE_SIZE);
            auto pages = bek::ceil_div(size, (uSize)PAGE_SIZE);
            if (auto region = mem::PageAllocator::the().allocate_region(pages)) {
                return {region->start.get(), region->size};
            } else {
                return {nullptr, 0};
            }
        }

        for (auto& allocator : m_slab_allocators) {
            if (size <= allocator.object_size() && align <= allocator.object_size()) {
                return {allocator.allocate(), allocator.object_size()};
            }
        }

        return {m_bitmap_allocator.allocate(size, align), size};
    }

    void free(void* ptr, uSize size, uSize align) {
        if (size >= 64 * KiB) {
            VERIFY(align <= PAGE_SIZE);
            VERIFY((uPtr)ptr % PAGE_SIZE == 0);
            mem::PageAllocator::the().free_region(mem::VirtualPtr{static_cast<u8*>(ptr)});
        }

        for (auto& allocator : m_slab_allocators) {
            if (size <= allocator.object_size() && align <= allocator.object_size()) {
                allocator.free(ptr);
                return;
            }
        }
        m_bitmap_allocator.free(ptr, size);
    }

    [[nodiscard]] uSize total_bytes() const { return m_bitmap_allocator.total_bytes(); }
    [[nodiscard]] uSize free_bytes() const {
        uSize free = m_bitmap_allocator.free_bytes();
        for (auto& allocator : m_slab_allocators) {
            free += allocator.free_bytes();
        }
        return free;
    }

    BitmapAllocator m_bitmap_allocator;
    SlabAllocator m_slab_allocators[6] = {SlabAllocator{32},  SlabAllocator{64},
                                          SlabAllocator{128}, SlabAllocator{256},
                                          SlabAllocator{512}, SlabAllocator{1024}};
};

// FIXME BAD BAD BAD
inline constexpr uSize INITIAL_ALLOCATION_SPACE_SIZE = 1 * MiB;
alignas(BitmapAllocator::chunk_size) static char initial_allocation_space
    [INITIAL_ALLOCATION_SPACE_SIZE];

bek::optional<KernelAllocator> global_kernel_allocator{};

void mem::initialise_kmalloc() {
    global_kernel_allocator =
        KernelAllocator{initial_allocation_space, INITIAL_ALLOCATION_SPACE_SIZE};
}

mem::AllocatedRegion mem::allocate(uSize size, uSize align) {
    auto res = global_kernel_allocator->allocate(size, align);
    if (!res.pointer) DBG::dbgln("Failed to allocate."_sv);
    DBG::dbgln("{} of {} bytes remaining."_sv, global_kernel_allocator->free_bytes(),
               global_kernel_allocator->total_bytes());
    return res;
}
void mem::free(void* ptr, uSize size, uSize align) {
    if (ptr != nullptr) {
        global_kernel_allocator->free(ptr, size, align);
    }
}
bek::pair<uSize, uSize> mem::get_kmalloc_usage() {
    return {global_kernel_allocator->free_bytes(), global_kernel_allocator->total_bytes()};
}
void mem::log_kmalloc_usage() {
    auto [free_mem, total_mem] = mem::get_kmalloc_usage();
    DBG::warnln("Memory: {} of {} bytes used ({}%)."_sv, total_mem - free_mem, total_mem,
                ((total_mem - free_mem) * 100) / total_mem);
}

void* operator new(uSize count) { return mem::allocate(count).pointer; }
void* operator new[](uSize count) { return mem::allocate(count).pointer; }
void* operator new(uSize count, std::align_val_t al) {
    return mem::allocate(count, static_cast<uSize>(al)).pointer;
}
void* operator new[](uSize count, std::align_val_t al) {
    return mem::allocate(count, static_cast<uSize>(al)).pointer;
}
void operator delete(void* ptr, uSize sz) noexcept { mem::free(ptr, sz); }
void operator delete[](void* ptr, uSize sz) noexcept { mem::free(ptr, sz); }
void operator delete(void* ptr, uSize sz, std::align_val_t al) noexcept {
    mem::free(ptr, sz, static_cast<uSize>(al));
}
void operator delete[](void* ptr, uSize sz, std::align_val_t al) noexcept {
    mem::free(ptr, sz, static_cast<uSize>(al));
}
