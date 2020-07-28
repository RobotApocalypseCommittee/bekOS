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

#ifndef BEKOS_UTILITY_H
#define BEKOS_UTILITY_H

#include <type_traits>
#include <stdint.h>
#include <hardtypes.h>
#include "library/liballoc.h"

inline void* operator new(size_t size)
{
    return PREFIX(malloc)(size);
}

inline void operator delete(void* ptr)
{
    return PREFIX(free)(ptr);
}

inline void operator delete(void* ptr, size_t)
{
    return PREFIX(free)(ptr);
}

inline void* operator new[](size_t size)
{
    return PREFIX(malloc)(size);
}

inline void operator delete[](void* ptr)
{
    return PREFIX(free)(ptr);
}

inline void operator delete[](void* ptr, size_t)
{
    return PREFIX(free)(ptr);
}

inline void* operator new(size_t, void* ptr)
{
    return ptr;
}

inline void* operator new[](size_t, void* ptr)
{
    return ptr;
}

namespace bek {

    template< class T >
    typename std::remove_reference<T>::type&& move( T&& t ) noexcept {
        return static_cast<typename std::remove_reference<T>::type&&>(t);
    }

    template<class T>
    void swap(T& a, T& b) noexcept {
        T tmp = move(a);
        a = move(b);
        b = move(tmp);
    }

    template<class T>
    u64 hash(T);

    template<>
    u64 hash(u64 x);

    template<class T>
    void copy(T* dest, const T* src, size_t n) {
        while (n > 0) {
            n--;
            *(dest++) = *(src++);
        }
    }

    template<class T>
    void copy_backward(T* dest, const T* src, size_t n) {
        dest += n - 1; src += n - 1;
        while (n > 0) {
            n--;
            *(dest--) = *(src--);
        }
    }

    template<class T, class U>
    struct pair {
        T first;
        U second;
    };
}

#endif //BEKOS_UTILITY_H
