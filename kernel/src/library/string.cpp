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

#include <kstring.h>
#include <library/assert.h>
#include "library/string.h"
#include "library/utility.h"

const u8 short_max_length = 14;

namespace bek {

    void swap(string &first, string &second) {
        /// Assume short occupies same space as long
        swap(first.m_short, second.m_short);
    }

    string::string(const string &s) {
        if (s.is_long()) {
            // Copy most of representation
            m_long.capacity = s.m_long.capacity;
            m_long.length = s.m_long.length;
            m_long.data = new char[m_long.capacity];
            copy(m_long.data, s.m_long.data, m_long.length + 1);
        } else {
            // Simply copy
            m_short = s.m_short;
        }
    }

    string::~string() {
        if (is_long()) delete[] m_long.data;
    }

    string::string(const char *source) {
        u32 len = strlen(source);
        if (len > short_max_length) {
            // Long string
            set_long_capacity(len + 1);
            set_long_length(len);
            m_long.data = new char[len + 1];
            strcpy(m_long.data, source);
        } else {
            set_short_length(len);
            strcpy(&m_short.in_data[0], source);
        }
    }

    string::string(u32 length, char init) {
        if (length > short_max_length) {
            set_long_length(length);
            set_long_capacity(length + 1);
            m_long.data = new char[length + 1];
            memset(m_long.data, init, length);
            m_long.data[length] = '\0';
        } else {
            set_short_length(length);
            memset(&m_short.in_data[0], init, length);
            m_short.in_data[length] = '\0';
        }
    }

    string &string::operator=(string s) {
        swap(*this, s);
        return *this;
    }

    string::string(string &&s) {
        swap(*this, s);
    }

    bool string::is_long() const {
        return m_short.length & 0x1;
    }

    u32 string::size() const {
        return is_long() ? m_long.length : get_short_length();
    }

    const char *string::data() const {
        return is_long() ? m_long.data : &m_short.in_data[0];
    }

    void string::set_short_length(u8 len) {
        assert(len <= 14);
        m_short.length = len << 1;
    }

    void string::set_long_length(u32 length) {
        m_long.length = length;
    }

    void string::set_long_capacity(u32 capacity) {
        assert(capacity < ((1 << 31) - 1));
        m_long.capacity = (capacity << 1) | 0x1;
    }

    u32 string::get_long_capacity() const {
        return m_long.capacity >> 1;
    }

    u8 string::get_short_length() const {
        return m_short.length >> 1;
    }

    string::string() {}
}