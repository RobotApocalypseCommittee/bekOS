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
#include "memory.h"
namespace mem {

// FIXME: This will need to depend on dma mappings for a specific device!
bek::optional<DmaPtr> virt_to_dma(void* ptr);

void dma_sync_before_read(const void* ptr, uSize size);
void dma_sync_after_write(const void* ptr, uSize size);

/// A view onto a contiguous space of dma-able memory. No access is provided to the underlying
/// memory.
class dma_view {
public:
    dma_view(DmaPtr dmaPtr, uSize i) : m_dma_ptr(dmaPtr), m_size{i} {}
    [[nodiscard]] DmaPtr dma_ptr() const { return m_dma_ptr; }

    [[nodiscard]] dma_view subdivide(uSize offset, uSize size) const {
        VERIFY(offset + size <= m_size);
        return {m_dma_ptr.offset(offset), size};
    }

    [[nodiscard]] constexpr uSize size() const { return m_size; }

private:
    DmaPtr m_dma_ptr;
    uSize m_size;
};

template <typename T>
class dma_array {
public:
    explicit dma_array(uSize count) : dma_array(count, alignof(T)) {}

    dma_array(uSize count, uSize align)
        : m_data(static_cast<T*>(kmalloc(count * sizeof(T), align))),
          m_count(count),
          m_align(align) {}

    dma_array(const dma_array&)            = delete;
    dma_array& operator=(const dma_array&) = delete;
    ~dma_array() { kfree(m_data, m_count * sizeof(T), m_align); }

    constexpr const T* data() const { return m_data; }
    constexpr T* data() { return m_data; }

    const T& operator[](uSize idx) const {
        ASSERT(idx < m_count);
        return data()[idx];
    }

    T& operator[](uSize idx) {
        ASSERT(idx < m_count);
        return data()[idx];
    }

    T* begin() { return data(); }
    T* end() { return &data()[m_count]; }

    [[nodiscard]] constexpr uSize size() const { return m_count; }
    [[nodiscard]] constexpr uSize byte_size() const { return m_count * sizeof(T); }

    [[nodiscard]] DmaPtr dma_ptr(uSize idx = 0) const {
        auto o_ptr = virt_to_dma(&m_data[idx]);
        VERIFY(o_ptr);
        return *o_ptr;
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
    T* m_data;
    uSize m_count;
    uSize m_align;
};

template <typename T>
class dma_object {
public:
    explicit dma_object() : m_object{static_cast<T*>(kmalloc(sizeof(T), alignof(T)))} {}
    ~dma_object() { kfree(m_object, sizeof(T), alignof(T)); }
    dma_object(const dma_object&)            = delete;
    dma_object& operator=(const dma_object&) = delete;
    dma_object(dma_object&& other) : m_object{bek::exchange(other.m_object, nullptr)} {}
    dma_object& operator=(dma_object&& other) {
        dma_object copy{bek::move(other)};
        bek::swap(m_object, copy.m_object);
        return *this;
    }

    const T& operator*() const { return *m_object; }
    T& operator*() { return *m_object; }

    const T* operator->() const { return m_object; }
    T* operator->() { return m_object; }

    void sync_before_read() { dma_sync_before_read(m_object, sizeof(T)); }

    void sync_after_write() {
        dma_sync_after_write(reinterpret_cast<const u8*>(m_object), sizeof(T));
    }

    [[nodiscard]] DmaPtr dma_ptr() const {
        auto o_ptr = virt_to_dma(m_object);
        VERIFY(o_ptr);
        return *o_ptr;
    }

    [[nodiscard]] dma_view get_dma_view() const { return {dma_ptr(), sizeof(T)}; }

private:
    T* m_object;
};

class dma_buffer {
public:
    explicit dma_buffer(uSize n) : buffer(static_cast<char*>(kmalloc(n)), n) {}
    dma_buffer(const dma_buffer&)            = delete;
    dma_buffer& operator=(const dma_buffer&) = delete;
    dma_buffer(dma_buffer&& other) : buffer(bek::exchange(other.buffer, {nullptr, 0})) {}
    dma_buffer& operator=(dma_buffer&& other) {
        dma_buffer new_other{bek::move(other)};
        bek::swap(buffer, new_other.buffer);
        return *this;
    }

    const bek::mut_buffer& get() const { return buffer; }
    bek::mut_buffer& get() { return buffer; }
    [[nodiscard]] DmaPtr dma_ptr() const {
        auto p = virt_to_dma(buffer.data());
        VERIFY(p);
        return *p;
    }

    [[nodiscard]] dma_view get_dma_view() const { return {dma_ptr(), buffer.size()}; }

    [[nodiscard]] uSize size() const { return buffer.size(); }

private:
    bek::mut_buffer buffer;
};

}  // namespace mem

#endif  // BEKOS_DMA_UTILS_H
