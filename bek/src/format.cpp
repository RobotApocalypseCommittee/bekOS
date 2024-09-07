/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
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

#include "bek/format.h"

void bek::StringStream::write(bek::str_view str) {
    // TODO: Make more efficient.
    chars.reserve(chars.size() + str.size());
    for (char c : str) {
        chars.push_back(c);
    }
}
void bek::StringStream::write(char c) { chars.push_back(c); }
void bek::StringStream::reserve(uSize n) { chars.reserve(n + chars.size()); }
bek::string bek::StringStream::build_and_clear() {
    chars.push_back('\0');
    // TODO: Construct without copy.
    bek::string result{chars.data()};
    chars.clear();
    return result;
}
void bek::internal::format_to(bek::OutputStream& out, bek::str_view format_str,
                              bek::span<TypeErasedFormatter> parameters) {
    uSize a_index     = 0;
    uSize b_index     = 0;
    const uSize end   = format_str.size();
    uSize param_index = 0;

    while (b_index < end) {
        if (format_str[b_index] == '{') {
            ASSERT(b_index + 1 < end);
            // Commit string so far.
            if (b_index > a_index) {
                out.write(format_str.substr(a_index, b_index - a_index));
            }

            // Move past bracket.
            b_index++;
            a_index = b_index;

            // Check for escape - if so, move onto next char.
            if (format_str[b_index] == '{') {
                b_index++;
            } else {
                // Read until closing brace
                while (b_index < end && format_str[b_index] != '}') {
                    b_index++;
                }

                auto format_spec = format_str.substr(a_index, b_index - a_index);
                ASSERT(param_index < parameters.size());
                parameters[param_index].format_fn(out, parameters[param_index].value, format_spec);
                param_index++;

                b_index++;
                a_index = b_index;
            }

        } else if (format_str[b_index] == '}') {
            // Must be an escape
            ASSERT(b_index + 1 < end && format_str[b_index + 1] == '}');
            a_index = b_index + 1;
            b_index = b_index + 2;
        } else {
            b_index++;
        }
    }

    // Tail end
    if (b_index > a_index) {
        out.write(format_str.substr(a_index, b_index - a_index));
    }
}
void bek::internal::UnsignedIntegralFormatter::parse(bek::str_view spec) {
    if (spec.size() == 0) return;
    ASSERT(spec.size() == 2 || spec.size() == 3);
    ASSERT(spec[0] == ':');
    switch (spec[1]) {
        case 'd':
            style = Style::Decimal;
            break;
        case 'x':
            style = Style::LowerHex;
            break;
        case 'X':
            style = Style::UpperHex;
            break;
        case 'b':
            style = Style::Binary;
            break;
    }
    if (spec.size() == 3) {
        if (spec[2] == 'L') {
            if (style == Style::Binary) {
                min_length = 64;
            } else if (style == Style::UpperHex || style == Style::LowerHex) {
                min_length = 16;
            }
        } else if (spec[2] == 'l') {
            if (style == Style::Binary) {
                min_length = 32;
            } else if (style == Style::UpperHex || style == Style::LowerHex) {
                min_length = 8;
            }
        }
    }
}

static bek::str_view convert_unsigned_to_string(u64 v, u8 base, bool uppercase, char (&buffer)[64],
                                                u8 min_len) {
    static constexpr const char* lowercase_alphabet = "0123456789abcdef";
    static constexpr const char* uppercase_alphabet = "0123456789ABCDEF";
    ASSERT(base >= 2 && base <= 16);
    ASSERT(min_len <= 64);
    bek::memset(buffer, '0', 64);

    if (v == 0) {
        return {&buffer[0], min_len};
    }

    // Funky reverse evading - is it necessary?
    uSize i = 64;
    do {
        i--;
        buffer[i] = (uppercase ? uppercase_alphabet : lowercase_alphabet)[v % base];
        v /= base;
    } while (i > 0 && v);
    if (64 - i < min_len) {
        i = 64 - min_len;
    }
    return {&buffer[i], 64 - i};
}

void bek::internal::UnsignedIntegralFormatter::format_to(bek::OutputStream& out, u64 v) {
    int base       = 10;
    bool uppercase = false;
    switch (style) {
        case Style::UpperHex:
            uppercase = true;
            [[fallthrough]];
        case Style::LowerHex:
            base = 16;
            break;
        case Style::Decimal:
            base = 10;
            break;
        case Style::Binary:
            base = 2;
            break;
    }
    char buffer[64];

    out.write(convert_unsigned_to_string(v, base, uppercase, buffer, min_length));
}

void bek::internal::SignedIntegralFormatter::format_to(bek::OutputStream& out, i64 v) {
    if (v < 0) out.write('-');
    UnsignedIntegralFormatter::format_to(out, static_cast<u64>(+v));
}
