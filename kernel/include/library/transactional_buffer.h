// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024 Bekos Contributors
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

#ifndef BEKOS_TRANSACTIONAL_BUFFER_H
#define BEKOS_TRANSACTIONAL_BUFFER_H

#include "bek/memory.h"
#include "bek/str.h"
#include "bek/types.h"
#include "kernel_error.h"

class TransactionalBuffer {
public:
    virtual uSize size() const = 0;
    [[nodiscard]] virtual expected<uSize> write_from(const void* buffer, uSize length, uSize offset) = 0;
    [[nodiscard]] virtual expected<uSize> read_to(void* buffer, uSize length, uSize offset) = 0;

    template <typename T>
    [[nodiscard]] expected<T> read_object(uSize offset = 0) {
        T obj{};
        EXPECTED_TRY(read_to(reinterpret_cast<void*>(&obj), sizeof(T), offset));
        return obj;
    }

    template <typename T>
    [[nodiscard]] expected<uSize> write_object(const T& obj, uSize offset = 0) {
        EXPECTED_TRY(write_from(reinterpret_cast<const void*>(&obj), sizeof(T), offset));
        return {0ull};
    }
};

class KernelBuffer : public TransactionalBuffer {
public:
    KernelBuffer(void* ptr, uSize size) : m_ptr{reinterpret_cast<uPtr>(ptr)}, m_size{size} {};
    uSize size() const override { return m_size; }
    expected<uSize> write_from(const void* buffer, uSize length, uSize offset) override {
        bek::memcopy(reinterpret_cast<void*>(m_ptr + offset), buffer, length);
        return length;
    }
    expected<uSize> read_to(void* buffer, uSize length, uSize offset) override {
        bek::memcopy(buffer, reinterpret_cast<const void*>(m_ptr + offset), length);
        return length;
    }

private:
    uPtr m_ptr;
    uSize m_size;
};

template <typename T>
class BitwiseObjectBuffer : public TransactionalBuffer {
public:
    explicit BitwiseObjectBuffer(const T& obj) : m_obj{obj} {}
    uSize size() const override { return sizeof(T); }
    T& object() { return m_obj; }

    expected<uSize> write_from(const void* buffer, uSize length, uSize offset) override {
        VERIFY(offset == 0 && length == sizeof(T));
        bek::memcopy(&m_obj, buffer, length);
        return length;
    }
    expected<uSize> read_to(void* buffer, uSize length, uSize offset) override {
        VERIFY(offset == 0 && length == sizeof(T));
        bek::memcopy(buffer, &m_obj, length);
        return length;
    }

private:
    T m_obj;
};

class TransactionalBufferSubset: public TransactionalBuffer {
public:
    explicit TransactionalBufferSubset(TransactionalBuffer& buffer, uSize offset, uSize length) : m_buffer(buffer), m_offset(offset), m_length(length) {
        VERIFY(m_offset <= buffer.size());
        VERIFY(m_offset + m_length <= buffer.size());
    }
    uSize size() const override { return m_length; };
    [[nodiscard]] expected<uSize> write_from(const void* buffer, uSize length, uSize offset) override {
        return m_buffer.write_from(buffer, length, offset + m_offset);
    };
    [[nodiscard]] expected<uSize> read_to(void* buffer, uSize length, uSize offset) override {
        return m_buffer.read_to(buffer, length, offset + m_offset);
    }

private:
    TransactionalBuffer& m_buffer;
    u64 m_offset;
    u64 m_length;
};

#endif  // BEKOS_TRANSACTIONAL_BUFFER_H
