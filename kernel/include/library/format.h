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

#ifndef BEKOS_FORMAT_H
#define BEKOS_FORMAT_H

#include "format_core.h"
#include "span.h"
#include "string.h"
namespace bek {

template <typename T>
struct Formatter;

struct StringStream : OutputStream {
    void write(bek::str_view str) override;
    void write(char c) override;
    void reserve(uSize n) override;
    bek::string build_and_clear();

    bek::vector<char> chars{};
};

}  // namespace bek

namespace bek::internal {

template <typename T>
concept has_basic_formatter = requires(bek::OutputStream& s, const T& t) {
    { bek_basic_format(s, t) } -> same_as<void>;
};

struct UnsignedIntegralFormatter {
    enum class Style {
        Decimal,
        UpperHex,
        LowerHex,
        Binary,
    };
    Style style{Style::Decimal};
    u8 min_length{1};
    void parse(bek::str_view spec);
    void format_to(OutputStream& out, u64 v);
};

struct SignedIntegralFormatter : private UnsignedIntegralFormatter {
    using UnsignedIntegralFormatter::parse;
    void format_to(OutputStream& out, i64 v);
};

struct StringFormatter {
    void parse(bek::str_view){};
    void format_to(OutputStream& out, bek::str_view str) { out.write(str); }
};

struct TypeErasedFormatter {
    void (*format_fn)(OutputStream& out, const void* value, bek::str_view fmt_string);
    const void* value;
};

template <typename T>
void formatter_fn(OutputStream& out, const void* value, bek::str_view fmt_string) {
    auto& val = *static_cast<const T*>(value);
    if constexpr (has_basic_formatter<T>) {
        bek_basic_format(out, val);
    } else {
        Formatter<T> formatter;
        formatter.parse(fmt_string);
        formatter.format_to(out, val);
    }
}

void format_to(OutputStream& out, bek::str_view format_str,
               bek::span<TypeErasedFormatter> parameters);

}  // namespace bek::internal

namespace bek {

template <>
struct Formatter<bek::str_view> : internal::StringFormatter {};

template <>
struct Formatter<char const*> : internal::StringFormatter {
    void format_to(OutputStream& out, const char* str) {
        internal::StringFormatter::format_to(out, bek::str_view{str});
    }
};

template <uSize Size>
struct Formatter<char[Size]> : Formatter<char const*> {};

template <>
struct Formatter<bek::string> : internal::StringFormatter {};

template <>
struct Formatter<u64> : internal::UnsignedIntegralFormatter {};

template <>
struct Formatter<u32> : internal::UnsignedIntegralFormatter {};

template <>
struct Formatter<u16> : internal::UnsignedIntegralFormatter {};

template <>
struct Formatter<u8> : internal::UnsignedIntegralFormatter {};

template <>
struct Formatter<i64> : internal::SignedIntegralFormatter {};

template <>
struct Formatter<i32> : internal::SignedIntegralFormatter {};

template <>
struct Formatter<i16> : internal::SignedIntegralFormatter {};

template <>
struct Formatter<i8> : internal::SignedIntegralFormatter {};

template <typename... Types>
void format_to(OutputStream& out, bek::str_view format_str, Types const&... parameters) {
    bek::internal::TypeErasedFormatter erased_params[sizeof...(Types)] = {
        {internal::formatter_fn<Types>, static_cast<const void*>(&parameters)}...};
    format_to(out, format_str,
              span<internal::TypeErasedFormatter>{erased_params, sizeof...(Types)});
}

template <typename... Types>
bek::string format(bek::str_view format_str, Types const&... parameters) {
    bek::StringStream out;
    format_to(out, format_str, parameters...);
    return out.build_and_clear();
}

}  // namespace bek

#endif  // BEKOS_FORMAT_H
