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

#ifndef BEKOS_OPTIONAL_H
#define BEKOS_OPTIONAL_H

#include "library/assertions.h"
#include "mm/kmalloc.h"
#include "types.h"
#include "utility.h"

namespace bek {

struct in_place_t {
    inline constexpr explicit in_place_t() = default;
};

template <typename T>
class optional {
public:
    constexpr optional() noexcept {}

    constexpr optional(const optional&) = default;
    constexpr optional(optional&&)      = default;

    constexpr optional(const optional& other)
        requires(!bek::is_trivially_copy_constructible<T>)
        : m_valid(other.m_valid) {
        if (m_valid) {
            initialise(*other);
        }
    }

    constexpr optional(optional&& other)
        requires(!bek::is_trivially_move_constructible<T>)
        : m_valid(other.m_valid) {
        if (m_valid) {
            initialise(move(*other));
        }
    }

    template <typename First, typename... Rest>
        requires(!same_as<remove_cv_reference<First>, optional>)
    constexpr optional(First&& first, Rest&&... rest) : m_valid(true) {
        initialise(forward<First&&>(first), forward<Rest&&>(rest)...);
    }

    friend void swap(optional& a, optional& b) {
        if constexpr (!bek::is_trivially_copyable<T>) {
            using bek::swap;
            if (a.m_valid && b.m_valid) {
                swap(*a, *b);
            } else if (a.m_valid && !b.m_valid) {
                b.initialise(bek::move(*a));
                a.reset();
            } else if (!a.m_valid && b.m_valid) {
                a.initialise(bek::move(*b));
                b.reset();
            }
        } else {
            using bek::swap;
            swap(a.m_valid, b.m_valid);
            swap(a.m_value, b.m_value);
        }
    }

    optional& operator=(optional other) {
        swap(*this, other);
        return *this;
    }

    explicit operator bool() const { return m_valid; }

    const T& operator*() const& {
        ASSERT(m_valid);
        return m_value;
    }
    T& operator*() & {
        ASSERT(m_valid);
        return m_value;
    }

    const T&& operator*() const&& {
        ASSERT(m_valid);
        return move(m_value);
    }
    T&& operator*() && {
        ASSERT(m_valid);
        return move(m_value);
    }

    constexpr const T* operator->() const {
        ASSERT(m_valid);
        return &m_value;
    }

    constexpr T* operator->() {
        ASSERT(m_valid);
        return &m_value;
    }

    [[nodiscard]] bool is_valid() const { return m_valid; }

    template <typename U>
    T value_or(U&& alt) const& {
        return (*this) ? **this : static_cast<T>(forward<U>(alt));
    }

    template <typename U>
    T value_or(U&& alt) && {
        return (*this) ? **this : static_cast<T>(forward<U>(alt));
    }

    ~optional() = default;
    ~optional()
        requires(!bek::is_trivially_destructible<T>)
    {
        if (m_valid) m_value.~T();
    }

private:
    template <typename... Args>
    inline constexpr void initialise(Args&&... args) {
        new (&m_value) T(forward<Args&&>(args)...);
        m_valid = true;
    }

    inline constexpr void reset() {
        if (m_valid) {
            m_valid = false;
            m_value.~T();
        }
    }

    union {
        char m_null_state{};
        T m_value;
    };
    bool m_valid{};
};

static_assert(bek::is_trivially_copy_constructible<bek::optional<int>>);
}  // namespace bek

#endif  // BEKOS_OPTIONAL_H
