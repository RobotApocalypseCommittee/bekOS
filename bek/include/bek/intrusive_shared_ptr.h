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

#ifndef BEKOS_INTRUSIVE_SHARED_PTR_H
#define BEKOS_INTRUSIVE_SHARED_PTR_H

#include "bek/assertions.h"
#include "bek/traits.h"
#include "bek/utility.h"

namespace bek {

template <typename T>
concept ref_counted = requires(T& x) {
    x.acquire();
    x.release();
};

template <typename T>
class shared_ptr {
public:
    struct AdoptTag {};

    constexpr shared_ptr() noexcept = default;
    constexpr shared_ptr(decltype(nullptr)) noexcept {}

    shared_ptr(T* ptr) : m_ptr{ptr} {
        static_assert(ref_counted<T>);
        if (m_ptr) m_ptr->acquire();
    }

    shared_ptr(AdoptTag, T* ptr) : m_ptr(ptr) { static_assert(ref_counted<T>); }

    template <typename U>
        requires bek::implictly_convertible_to<U*, T*>
    shared_ptr(U* ptr) : m_ptr{ptr} {  // NOLINT(*-explicit-constructor)
        static_assert(ref_counted<T>);
        if (m_ptr) m_ptr->acquire();
    }

    shared_ptr(const shared_ptr& ptr) : m_ptr{ptr.m_ptr} {
        if (m_ptr) m_ptr->acquire();
    }

    shared_ptr(shared_ptr&& ptr) : m_ptr{bek::exchange(ptr.m_ptr, nullptr)} {}

    template <typename U>
        requires bek::implictly_convertible_to<U*, T*>
    shared_ptr(const shared_ptr<U>& ptr) : m_ptr{ptr.get()} {
        if (m_ptr) m_ptr->acquire();
    }

    friend void swap(shared_ptr& a, shared_ptr& b) noexcept {
        using bek::swap;
        swap(a.m_ptr, b.m_ptr);
    }

    shared_ptr& operator=(const shared_ptr& ptr) {
        shared_ptr temp{ptr};
        swap(temp, *this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& ptr) {
        shared_ptr temp{bek::move(ptr)};
        swap(temp, *this);
        return *this;
    }

    explicit operator bool() { return m_ptr != nullptr; }

    ~shared_ptr() {
        if (m_ptr) m_ptr->release();
    }

    T& operator*() const {
        ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const {
        ASSERT(m_ptr);
        return m_ptr;
    }
    T* get() const { return m_ptr; }

    friend bool operator==(const shared_ptr& a, const T* b) { return a.get() == b; }
    friend bool operator==(const shared_ptr&, const shared_ptr&) = default;

private:
    T* m_ptr{nullptr};
};

template <typename T>
class RefCounted {
public:
    void acquire() {
        VERIFY(m_ref_count != 0);
        m_ref_count++;
    }

    void release() {
        VERIFY(m_ref_count > 0);
        m_ref_count--;
        if (m_ref_count == 0) {
            delete (T*)this;
        }
    }

    u32 ref_count() const { return m_ref_count; }

protected:
    // FIXME: Make Mutable Atomic
    u32 m_ref_count{1};
};

template <ref_counted T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return {typename shared_ptr<T>::AdoptTag{}, new T{args...}};
}

template <ref_counted T>
shared_ptr<T> adopt_shared(T* ptr) {
    return {typename shared_ptr<T>::AdoptTag{}, ptr};
}

}  // namespace bek

#endif  // BEKOS_INTRUSIVE_SHARED_PTR_H
