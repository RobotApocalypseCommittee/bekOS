/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_STRING_H
#define BEKOS_STRING_H

#include "types.h"

namespace bek {

class string_view {
public:
    /// Length of string (not including null terminator)
    string_view(const char *string, uSize length);

    /// Must be null-terminated
    string_view(const char *string);

    string_view(const string_view &v) = default;

    string_view &operator=(const string_view &v) = default;

    [[nodiscard]] const char *data() const;

    [[nodiscard]] uSize size() const;

    [[nodiscard]] string_view substr(uSize pos, uSize len) const;

    [[nodiscard]] const char *begin() const;

    [[nodiscard]] const char *end() const;

    void remove_prefix(uSize n);

    void remove_suffix(uSize n);

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

    /// Excludes null-terminator
    u32 size() const {
        return is_long() ? m_long.length : get_short_length();
    }

    const char *data() const {
        return is_long() ? m_long.data : &m_short.in_data[0];
    };

    string(const string &);

    string(string &&);

    string &operator=(string);
    void swap(string &other);

    ~string();

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

}

#endif //BEKOS_STRING_H
