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
using namespace bek;

bek::string::string(const string &s) {
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

bek::string::~string() {
    if (is_long()) delete[] m_long.data;
}

bek::string::string(const char *source) {
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

bek::string::string(u32 length, char init) {
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

string &bek::string::operator=(string s) {
    swap(s);
    return *this;
}

bek::string::string(string &&s) { swap(s); }


void bek::string::set_short_length(u8 len) {
    assert(len <= 14);
    m_short.length = len << 1;
}

void bek::string::set_long_length(u32 length) {
    m_long.length = length;
}

void bek::string::set_long_capacity(u32 capacity) {
    assert(capacity < (unsigned(1 << 31) - 1));
    m_long.capacity = (capacity << 1) | 0x1;
}

u32 bek::string::get_long_capacity() const { return m_long.capacity >> 1; }

u8 bek::string::get_short_length() const { return m_short.length >> 1; }

bek::string::string() {}

void string::swap(string &s) { bek::swap(m_short, s.m_short); }

bek::string_view::string_view(const char *string, uSize length) : m_data{string}, m_size{length} {}

bek::string_view::string_view(const char *string) : m_data(string), m_size(strlen(string)) {}

const char *bek::string_view::data() const { return m_data; }

string_view bek::string_view::substr(uSize pos, uSize len) const {
    assert(pos <= size());
    return string_view{m_data + pos, bek::min(len, size() - pos)};
}

uSize bek::string_view::size() const {
    return m_size;
}

const char *bek::string_view::begin() const {
    return data();
}

const char *bek::string_view::end() const {
    return data() + size();
}

void string_view::remove_prefix(uSize n) {
    assert(n <= size());
    m_data += n;
    m_size -= n;
}

void string_view::remove_suffix(uSize n) {
    assert(n <= size());
    m_size -= n;
}
