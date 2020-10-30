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
#ifndef BEKOS_OWN_PTR_H
#define BEKOS_OWN_PTR_H

#include <library/utility.h>

namespace bek {
template <typename T>
class own_ptr {
public:
    own_ptr() noexcept = default;
    explicit own_ptr(T* ptr) : m_ptr(ptr) {}
    ~own_ptr() {
        if (m_ptr) delete m_ptr;
    }
    own_ptr(const own_ptr& other) = delete;
    own_ptr(own_ptr&& other) noexcept : m_ptr(other.release()) {}

    template <typename U, class = typename std::enable_if<std::is_convertible_v<T*, U*>>>
    own_ptr(own_ptr<U>&& other) noexcept : m_ptr(other.release()) {}

    own_ptr& operator=(const own_ptr&) = delete;
    own_ptr& operator                  =(own_ptr&& other) noexcept {
        swap(other);
        return *this;
    }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    T* release() noexcept {
        auto* tmp = m_ptr;
        m_ptr     = nullptr;
        return tmp;
    }
    void swap(own_ptr& other) noexcept {
        using bek::swap;
        swap(m_ptr, other.m_ptr);
    }
    T& operator*() {
        assert(m_ptr);
        return *m_ptr;
    }
    T* operator->() {
        assert(m_ptr);
        return m_ptr;
    }

private:
    T* m_ptr{nullptr};
};

template <typename T>
void swap(own_ptr<T>& a, own_ptr<T>& b) {
    a.swap(b);
}

template <typename T, typename... Args>
own_ptr<T> make_own(Args&&... args) {
    return own_ptr(new T(forward<Args>(args)...));
}

}  // namespace bek

#endif  // BEKOS_OWN_PTR_H
