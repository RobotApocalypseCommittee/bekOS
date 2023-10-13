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

#include "library/string.h"

#include <kstring.h>
#include <library/assertions.h>

#include "library/utility.h"
#include "mm/kmalloc.h"

const u8 short_max_length = 14;
using namespace bek;

bek::string::string(const string &s) {
    if (s.is_long()) {
        // Copy most of representation
        m_long.capacity = s.m_long.capacity;
        m_long.length = s.m_long.length;
        m_long.data     = static_cast<char *>(kmalloc(get_long_capacity()));
        copy(m_long.data, s.m_long.data, m_long.length + 1);
    } else {
        // Simply copy
        m_short = s.m_short;
    }
}

bek::string::~string() {
    if (is_long()) kfree(m_long.data, get_long_capacity());
}

bek::string::string(const char *source) {
    u32 len = strlen(source);
    if (len > short_max_length) {
        // Long string
        set_long_capacity(len + 1);
        set_long_length(len);
        m_long.data = static_cast<char *>(kmalloc(len + 1));
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
        m_long.data = static_cast<char *>(kmalloc(length + 1));
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
    ASSERT(len <= 14);
    m_short.length = len << 1;
}

void bek::string::set_long_length(u32 length) {
    m_long.length = length;
}

void bek::string::set_long_capacity(u32 capacity) {
    ASSERT(capacity < (unsigned(1 << 31) - 1));
    m_long.capacity = (capacity << 1) | 0x1;
}

u32 bek::string::get_long_capacity() const { return m_long.capacity >> 1; }

u8 bek::string::get_short_length() const { return m_short.length >> 1; }

bek::string::string() {}

void string::swap(string &s) { bek::swap(m_short, s.m_short); }
string::string(bek::str_view str) {
    // Without 0-terminator.
    auto len = str.size();
    if (len > short_max_length) {
        // Long string
        set_long_capacity(len + 1);
        set_long_length(len);
        m_long.data = static_cast<char *>(kmalloc(len + 1));
        memcpy(m_long.data, str.data(), len);
        m_long.data[len] = '\0';
    } else {
        set_short_length(len);
        memcpy(&m_short.in_data[0], str.data(), len);
        m_short.in_data[len] = '\0';
    }
}
bool bek::operator==(const string &a, const string &b) {
    if (a.size() != b.size()) return false;
    auto a_data = a.data();
    auto b_data = b.data();
    for (uSize i = 0; i < a.size(); i++) {
        if (a_data[i] != b_data[i]) return false;
    }
    return true;
}

bek::str_view::str_view(const char *string) : m_data(string), m_size(strlen(string)) {}

const char *bek::str_view::data() const { return m_data; }

str_view bek::str_view::substr(uSize pos, uSize len) const {
    ASSERT(pos <= size());
    return str_view{m_data + pos, bek::min(len, size() - pos)};
}

uSize bek::str_view::size() const { return m_size;
}

const char *bek::str_view::begin() const { return data();
}

const char *bek::str_view::end() const { return data() + size();
}

void str_view::remove_prefix(uSize n) {
    ASSERT(n <= size());
    m_data += n;
    m_size -= n;
}

void str_view::remove_suffix(uSize n) {
    ASSERT(n <= size());
    m_size -= n;
}
bool str_view::operator==(const str_view &b) const {
    if (size() != b.size()) return false;

    for (uSize i = 0; i < size(); i++) {
        if (m_data[i] != b.m_data[i]) return false;
    }
    return true;
}
u64 bek::hash(const str_view &view) { return bek::hash(view.data(), view.size()); }
u64 bek::hash(const string &str) { return bek::hash(str.data(), str.size()); }
