// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
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

#ifndef BEK_EXPECTED_H
#define BEK_EXPECTED_H

#include "swl/variant.hpp"

namespace bek {
/// Expected value of type T, or error value of type E.
template <typename T, typename E>
class expected {
    static_assert(!bek::is_same<bek::remove_cv_reference<T>, bek::remove_cv_reference<E>>);

public:
    template <typename U>
        requires bek::constructible<T, U> || bek::constructible<E, U>
    expected(U&& u) : m_variant{bek::forward<U>(u)} {}

    bool has_value() const { return swl::holds_alternative<T>(m_variant); }

    bool has_error() const { return swl::holds_alternative<E>(m_variant); }

    explicit operator bool() const { return has_value(); }

    const T& value() const {
        VERIFY(has_value());
        return swl::unsafe_get<T>(m_variant);
    }

    T& value() {
        VERIFY(has_value());
        return swl::unsafe_get<T>(m_variant);
    }

    const E& error() const {
        VERIFY(has_error());
        return swl::unsafe_get<E>(m_variant);
    }

    E& error() {
        VERIFY(has_error());
        return swl::unsafe_get<E>(m_variant);
    }

    T release_value() {
        VERIFY(has_value());
        return bek::move(swl::unsafe_get<T>(m_variant));
    }

    template <typename F, typename U = decltype(declval<F>()(declval<T>()))>
    expected<U, E> map_value(F&& fn) {
        if (has_value()) {
            return {fn(value())};
        } else {
            return {error()};
        }
    }

    expected(expected&&) = default;
    expected(const expected&) = default;
    expected& operator=(expected&&) = default;
    expected& operator=(const expected&) = default;

private:
    swl::variant<T, E> m_variant;
};

}  // namespace bek

#define EXPECTED_TRY(EXPRESSION)     \
    ({                               \
        auto&& _temp = (EXPRESSION); \
        if (_temp.has_error()) {     \
            return _temp.error();    \
        }                            \
        _temp.release_value();       \
    })

#define EXPECT_SUCCESS(EXPRESSION)                               \
    do {                                                         \
        if (ErrorCode _temp = (EXPRESSION); _temp != ESUCCESS) { \
            return _temp;                                        \
        }                                                        \
    } while (0)



#endif  // BEK_EXPECTED_H
