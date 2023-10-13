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

#ifndef BEKOS_STRING_H
#define BEKOS_STRING_H

#include "assertions.h"
#include "types.h"

namespace bek {

class str_view {
public:
    /// String may not contain null-chars. Length excludes terminating null-char (if present).
    constexpr str_view(const char *string, uSize length) : m_data{string}, m_size{length} {}

    /// Uses `strlen` to determine length, but does not include null-terminator.
    explicit str_view(char const *string);

    str_view(const str_view &v) = default;

    str_view &operator=(const str_view &v) = default;

    [[nodiscard]] const char *data() const;

    [[nodiscard]] uSize size() const;

    [[nodiscard]] str_view substr(uSize pos, uSize len) const;

    [[nodiscard]] const char *begin() const;

    [[nodiscard]] const char *end() const;

    char operator[](uSize i) const {
        ASSERT(i < m_size);
        return m_data[i];
    }

    void remove_prefix(uSize n);

    void remove_suffix(uSize n);

    bool operator==(const str_view &b) const;

private:
    const char *m_data;
    uSize m_size;
};

/// String w/ short string optimisation, cos why not?
class string {
public:

    string();

    explicit string(const char *source);

    /// length excludes null-terminator
    explicit string(u32 length, char init);

    explicit string(bek::str_view str);

    /// Excludes null-terminator
    u32 size() const {
        return is_long() ? m_long.length : get_short_length();
    }

    const char *data() const {
        return is_long() ? m_long.data : &m_short.in_data[0];
    };

    str_view view() const { return {data(), size()}; }

    string(const string &);

    string(string &&);

    string &operator=(string);
    void swap(string &other);

    ~string();

    friend bool operator==(const string &, const string &);

private:
    struct long_str {
        /// Allocated byte capacity, includes null terminator
        /// Assuming little endian, store 1 always in LSBit -> leftmost byte in memory
        u32 capacity;
        /// String length, exclude null terminator
        u32 length;
        /// Pointer to data
        char *data;
    };

    struct short_str {
        /// LSBit is 0, shifted
        u8 length;
        /// Short string capacity
        char in_data[15];
    };

    union {
        long_str m_long{};
        short_str m_short;
    };

    bool is_long() const {
        return m_short.length & 0x1;
    };

    void set_short_length(u8 len);

    void set_long_length(u32 length);

    void set_long_capacity(u32 capacity);

    u32 get_long_capacity() const;

    u8 get_short_length() const;
};

u64 hash(const bek::str_view &view);
u64 hash(const bek::string &str);

}  // namespace bek
constexpr bek::str_view operator""_sv(const char *str, uSize size) { return {str, size}; }

#endif //BEKOS_STRING_H
