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

#ifndef BEK_VECTOR_H
#define BEK_VECTOR_H

#include "allocations.h"
#include "buffer.h"
#include "initializer_list.h"
#include "memory.h"
#include "utility.h"

namespace bek {
template <class T>
class vector {
public:
    vector() = default;

    explicit vector(uSize size) : m_size(size), m_capacity(size) {
        auto res = mem::allocate(size * sizeof(T));
        VERIFY(res.pointer);
        array      = static_cast<T*>(res.pointer);
        m_capacity = res.size / sizeof(T);
        ASSERT(m_capacity >= m_size);
        for (uSize i = 0; i < size; i++) {
            new (&array[i]) T();
        }
    }

    vector(const vector& v) //__attribute((deprecated("Do you want to copy vector?")))
    {
        m_size     = v.m_size;
        m_capacity = v.m_capacity;
        if (m_capacity) {
            array      = reinterpret_cast<T*>(mem::allocate(m_capacity * sizeof(T)).pointer);
            for (uSize i = 0; i < m_size; i++) {
                new (&array[i]) T(v[i]);
            }
        }
    }

    vector(const T* begin, const T* end)
        : m_size{static_cast<uSize>(end - begin)}, m_capacity{m_size} {
        if (m_capacity) {
            auto res = mem::allocate(m_capacity * sizeof(T));
            VERIFY(res.pointer);
            array = reinterpret_cast<T*>(res.pointer);
            m_capacity = res.size / sizeof(T);
            for (auto* it = array; begin < end; it++, begin++) {
                new (it) T(*begin);
            }
        }
    }

    vector(vector&& v) noexcept : vector() { swap(v); }

    vector(std::initializer_list<T> list) {
        reserve(list.size());
        for (auto& item : list) push_back(item);
    }

    vector& operator=(const vector& v) __attribute((deprecated("Do you want to copy vector?"))) {
        if (this != &v) {
            clear();
            reserve(v.m_size);
            for (uSize i = 0; i < m_size; i++) {
                new (&array[i]) T(v[i]);
            }
            m_size = v.m_size;
        }
        return *this;
    }

    vector& operator=(vector&& v) noexcept {
        swap(v);
        return *this;
    }

    void swap(vector& other) noexcept {
        bek::swap(m_capacity, other.m_capacity);
        bek::swap(m_size, other.m_size);
        bek::swap(array, other.array);
    }

    void reserve(uSize n) {
        ensure_capacity(n);
    }

    void expand(uSize n) {
        reserve(n + size());
        m_size = n + size();
    }

    void push_back(const T& item) { push_back(T(item)); }

    void push_back(T&& item) {
        ensure_capacity(m_size + 1);
        new (&array[m_size]) T(bek::move(item));
        m_size++;
    }

    void insert(uSize i, const T& item) { insert(i, T{item}); }

    void insert(uSize i, T&& item) {
        VERIFY(i < size());
        ensure_capacity(m_size + 1);
        for (uSize idx = m_size - 1; i <= idx; idx--) {
            new (&array[idx + 1]) T(bek::move(array[idx]));
            array[idx].~T();
        }
        new (&array[i]) T(bek::move(item));
        m_size++;
    }

    T extract(const T& item) {
        const T* ptr = &item;
        VERIFY(ptr >= &array[0] && ptr < &array[m_size]);
        auto i = ptr - &array[0];
        T x = bek::move(array[i]);
        for (uSize idx = i + 1; idx < m_size; idx++) {
            array[idx - 1] = bek::move(array[idx]);
        }
        array[m_size - 1].~T();
        m_size--;
        return x;
    }

    T pop(uSize i) {
        VERIFY(i < m_size);
        T x = bek::move(array[i]);
        for (uSize idx = i + 1; idx < m_size; idx++) {
            array[idx - 1] = bek::move(array[idx]);
        }
        array[m_size - 1].~T();
        m_size--;
        return x;
    }

    T pop() { return pop(m_size - 1); }

    const T& operator[](uSize n) const { return array[n]; }

    T& operator[](uSize n) { return array[n]; }

    uSize size() const { return m_size; }

    T* data() { return &array[0]; }

    const T* data() const { return &array[0]; }

    void clear() {
        for (uSize i = 0; i < m_size; i++) {
            array[i].~T();
        }
        m_size = 0;
    }

    T* begin() { return data(); }
    const T* begin() const { return data(); }

    T* end() { return data() + size(); }
    const T* end() const { return data() + size(); }

    const T& back() const { return data()[size() - 1]; }
    T& back() { return data()[size() - 1]; }

    ~vector() {
        clear();
        mem::free(array, m_capacity * sizeof(T));
    }

    constexpr bek::mut_buffer buffer() {
        return {reinterpret_cast<char*>(data()), m_size * sizeof(T)};
    }

    constexpr bek::buffer buffer() const {
        return {reinterpret_cast<char*>(data()), m_size * sizeof(T)};
    }

private:
    ALWAYS_INLINE void ensure_capacity(uSize capacity) {
        uSize new_capacity = m_capacity ? m_capacity : 1u;
        while (new_capacity < capacity) {
            new_capacity *= 2;
        }
        if (new_capacity != m_capacity) {
            expand_storage(new_capacity);
        }
    }

    void expand_storage(uSize new_capacity) {
        auto* new_array = reinterpret_cast<T*>(mem::allocate(new_capacity * sizeof(T)).pointer);
        if (array) {
            if constexpr (bek::is_trivially_copyable<T>) {
                bek::memcopy(new_array, array, m_size * sizeof(T));
            } else {
                for (uSize i = 0; i < m_size; i++) {
                    new (&new_array[i]) T(bek::move(array[i]));
                    array[i].~T();
                }
            }
            mem::free(array, m_capacity * sizeof(T));
        }
        array = new_array;
        m_capacity = new_capacity;
    }

    uSize m_size{};
    uSize m_capacity{};
    T* array{};
};
}  // namespace bek

#endif  // BEK_VECTOR_H
