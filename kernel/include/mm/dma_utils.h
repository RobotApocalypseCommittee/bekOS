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

#ifndef BEKOS_DMA_UTILS_H
#define BEKOS_DMA_UTILS_H

#include "addresses.h"
#include "kmalloc.h"
#include "library/buffer.h"
#include "library/optional.h"
#include "library/vector.h"
#include "memory.h"
#include "peripherals/device_tree.h"

namespace mem {
void dma_sync_before_read(const void* ptr, uSize size);
void dma_sync_after_write(const void* ptr, uSize size);

class dma_pool;

class dma_buffer {
public:
    constexpr dma_buffer(char* data, uSize size, const DmaPtr& dma_ptr)
        : m_data(data), m_size(size), m_dma_ptr(dma_ptr) {}
    constexpr DmaPtr dma_ptr() const { return m_dma_ptr; }
    constexpr char* data() const { return m_data; }
    constexpr char* end() const { return m_data + m_size; }
    constexpr uSize size() const { return m_size; }
    constexpr bek::mut_buffer view() const { return {m_data, m_size}; }

    constexpr dma_buffer subdivide(uSize offset, uSize size) const {
        ASSERT(offset + size <= m_size);
        return {m_data + offset, size, m_dma_ptr.offset(offset)};
    }

    template <typename T>
    T& get_at(uSize offset) const {
        ASSERT(offset + sizeof(T) <= m_size);
        return *reinterpret_cast<T*>(data() + offset);
    }

    constexpr static dma_buffer null_buffer() { return {nullptr, 0, {0}}; }

private:
    char* m_data;
    uSize m_size;
    DmaPtr m_dma_ptr;
};

class own_dma_buffer {
public:
    constexpr DmaPtr dma_ptr() const { return m_buffer.dma_ptr(); }
    constexpr char* data() const { return m_buffer.data(); }
    constexpr char* end() const { return m_buffer.end(); }
    constexpr uSize size() const { return m_buffer.size(); }
    constexpr uSize align() const { return m_align; }
    constexpr dma_buffer view() const { return m_buffer; }
    constexpr bek::mut_buffer raw_view() const { return m_buffer.view(); }

    own_dma_buffer(const own_dma_buffer&)            = delete;
    own_dma_buffer& operator=(const own_dma_buffer&) = delete;
    own_dma_buffer(own_dma_buffer&& other)
        : m_buffer(bek::exchange(other.m_buffer, dma_buffer::null_buffer())),
          m_pool(other.m_pool),
          m_align(other.m_align) {}
    own_dma_buffer& operator=(own_dma_buffer&& other);

    void release() { m_buffer = dma_buffer::null_buffer(); }

    ~own_dma_buffer();

    own_dma_buffer(dma_pool& pool, dma_buffer buffer, uSize align)
        : m_buffer(buffer), m_pool(&pool), m_align(align) {}

private:
    dma_buffer m_buffer;
    dma_pool* m_pool;
    uSize m_align;
    friend class dma_pool;
};

class dma_pool {
public:
    virtual own_dma_buffer allocate(uSize size, uSize align) = 0;
    virtual void deallocate(const own_dma_buffer& buffer)    = 0;
};

inline own_dma_buffer& own_dma_buffer::operator=(own_dma_buffer&& other) {
    if (&other != this) {
        m_pool->deallocate(bek::move(*this));
        m_buffer = bek::exchange(other.m_buffer, dma_buffer::null_buffer());
        m_align  = other.m_align;
        m_pool   = other.m_pool;
    }
    return *this;
}

inline own_dma_buffer::~own_dma_buffer() {
    if (m_buffer.data()) {
        m_pool->deallocate(*this);
    }
}

template <typename T>
class dma_array {
public:
    dma_array(dma_pool& pool, uSize count)
        : dma_array(pool, count, alignof(T)) {}  // NOLINT(*-pro-type-member-init)

    dma_array(dma_pool& pool, uSize count, uSize align)
        : m_buffer(pool.allocate(count * sizeof(T), align)) {}

    constexpr const T* data() const { return reinterpret_cast<const T*>(m_buffer.data()); }
    constexpr T* data() { return reinterpret_cast<T*>(m_buffer.data()); }

    const T& operator[](uSize idx) const {
        ASSERT(idx < size());
        return data()[idx];
    }

    T& operator[](uSize idx) {
        ASSERT(idx < size());
        return data()[idx];
    }

    T* begin() { return data(); }
    T* end() { return &data()[size()]; }

    [[nodiscard]] constexpr uSize size() const { return m_buffer.size() / sizeof(T); }
    [[nodiscard]] constexpr uSize byte_size() const { return m_buffer.size(); }

    [[nodiscard]] constexpr DmaPtr dma_ptr(uSize idx = 0) const {
        return m_buffer.dma_ptr().offset(idx * sizeof(T));
    }

    void sync_before_read(uSize specific_index = -1ul) {
        auto* region_ptr = (specific_index == -1ul) ? data() : &data()[specific_index];
        auto region_size = (specific_index == -1ul) ? byte_size() : sizeof(T);
        dma_sync_before_read(region_ptr, region_size);
    }

    void sync_after_write(uSize specific_index = -1ul) {
        auto* region_ptr = (specific_index == -1ul) ? data() : &data()[specific_index];
        auto region_size = (specific_index == -1ul) ? byte_size() : sizeof(T);
        dma_sync_after_write(region_ptr, region_size);
    }

private:
    own_dma_buffer m_buffer;
};

template <typename T>
class dma_object {
public:
    explicit dma_object(dma_pool& pool) : m_buffer(pool.allocate(sizeof(T), alignof(T))) {}

    const T& operator*() const { return *reinterpret_cast<const T*>(m_buffer.data()); }
    T& operator*() { return *reinterpret_cast<T*>(m_buffer.data()); }

    const T* operator->() const { return reinterpret_cast<const T*>(m_buffer.data()); }
    T* operator->() { return reinterpret_cast<T*>(m_buffer.data()); }

    void sync_before_read() { dma_sync_before_read(m_buffer.data(), sizeof(T)); }

    void sync_after_write() { dma_sync_after_write(m_buffer.data(), sizeof(T)); }

    [[nodiscard]] constexpr DmaPtr dma_ptr() const { return m_buffer.dma_ptr(); }

    [[nodiscard]] dma_buffer get_view() const { return m_buffer.view(); }

private:
    own_dma_buffer m_buffer;
};

class MappedDmaPool final : public mem::dma_pool {
public:
    explicit MappedDmaPool(bek::vector<dev_tree::range_t> mappings)
        : m_mappings(bek::move(mappings)) {}
    mem::own_dma_buffer allocate(uSize size, uSize align) override;
    void deallocate(const mem::own_dma_buffer& buffer) override;

private:
    bek::vector<dev_tree::range_t> m_mappings;
};

}  // namespace mem

#endif  // BEKOS_DMA_UTILS_H
