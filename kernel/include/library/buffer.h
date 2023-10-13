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

#ifndef BEKOS_LIBRARY_BUFFER_H
#define BEKOS_LIBRARY_BUFFER_H

#include "assertions.h"
#include "types.h"
#include "utility.h"

namespace bek {

/// Represents contiguous section of mutable bytes.
struct mut_buffer {
    constexpr mut_buffer(char* data, uSize size) : m_data{data}, m_size{size} {}

    [[nodiscard]] char* data() const { return m_data; }

    [[nodiscard]] char* end() const { return m_data + m_size; }

    [[nodiscard]] uSize size() const { return m_size; }

    [[nodiscard]] mut_buffer subdivide(uSize byte_offset, uSize size) const {
        ASSERT(byte_offset + size <= m_size);
        return {m_data + byte_offset, size};
    }

    /// Gets the provided type via reinterpreting bytes at offset.
    template <typename T>
    T& get_at(uSize byte_offset) const {
        ASSERT(byte_offset + sizeof(T) <= m_size);
        return *reinterpret_cast<T*>(m_data + byte_offset);
    }

    /// Gets the provided type via reinterpreting bytes at offset, in a little-endian manner.
    template <typename T>
    [[nodiscard]] T get_at_le(uSize byte_offset) const {
        auto x = get_at<T>(byte_offset);
        if constexpr (bek::IS_LITTLE_ENDIAN) {
            return x;
        } else {
            return bek::swapBytes(x);
        }
    }

    /// Gets the provided type via reinterpreting bytes at offset, in a big-endian manner.
    template <typename T>
    [[nodiscard]] T get_at_be(uSize byte_offset) const {
        auto x = get_at<T>(byte_offset);
        if constexpr (bek::IS_LITTLE_ENDIAN) {
            return bek::swapBytes(x);
        } else {
            return x;
        }
    }

private:
    char* m_data;
    uSize m_size;
};

/// Represents contiguous section of immutable bytes.
struct buffer {
    constexpr buffer(const char* data, uSize size) : m_data{data}, m_size{size} {}

    buffer(mut_buffer buf)
        : m_data{buf.data()}, m_size{buf.size()} {}  // NOLINT(*-explicit-constructor)

    [[nodiscard]] const char* data() const { return m_data; }

    [[nodiscard]] const char* end() const { return m_data + m_size; }

    [[nodiscard]] constexpr uSize size() const { return m_size; }

    [[nodiscard]] constexpr buffer subdivide(uSize byte_offset, uSize size) const {
        ASSERT(byte_offset + size <= m_size);
        return {m_data + byte_offset, size};
    }

    /// Gets the provided type via reinterpreting bytes at offset.
    template <typename T>
    const T& get_at(uSize byte_offset) const {
        ASSERT(byte_offset + sizeof(T) <= m_size);
        return *reinterpret_cast<const T*>(m_data + byte_offset);
    }

    /// Gets the provided type via reinterpreting bytes at offset, in a little-endian manner.
    template <typename T>
    [[nodiscard]] T get_at_le(uSize byte_offset) const {
        auto x = get_at<T>(byte_offset);
        if constexpr (bek::IS_LITTLE_ENDIAN) {
            return x;
        } else {
            return bek::swapBytes(x);
        }
    }

    /// Gets the provided type via reinterpreting bytes at offset, in a big-endian manner.
    template <typename T>
    [[nodiscard]] T get_at_be(uSize byte_offset) const {
        auto x = get_at<T>(byte_offset);
        if constexpr (bek::IS_LITTLE_ENDIAN) {
            return bek::swapBytes(x);
        } else {
            return x;
        }
    }

    constexpr bool operator==(const buffer&) const = default;

private:
    const char* m_data;
    uSize m_size;
};
}

#endif // BEKOS_LIBRARY_BUFFER_H

