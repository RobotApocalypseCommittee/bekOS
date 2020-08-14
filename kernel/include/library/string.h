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

#include <hardtypes.h>

namespace bek {

/// String w/ short string optimisation, cos why not?
    class string {
    public:

        string();
        explicit string(const char* source);

        /// length excludes null-terminator
        explicit string(u32 length, char init);

        /// Excludes null-terminator
        inline u32 size() const;

        inline const char* data() const;

        string(const string&);
        string(string&&);
        string& operator=(string);
        ~string();

        friend void swap(string& first, string& second);
    private:
        struct long_str {
            /// Allocated byte capacity, includes null terminator
            /// Assuming little endian, store 1 always in LSBit -> leftmost byte in memory
            u32 capacity;
            /// String length, exclude null terminator
            u32 length;
            /// Pointer to data
            char* data;
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

        inline bool is_long() const;

        inline void set_short_length(u8 len);
        inline void set_long_length(u32 length);
        inline void set_long_capacity(u32 capacity);
        inline u32  get_long_capacity() const;
        inline u8   get_short_length() const;
    };

}

#endif //BEKOS_STRING_H
