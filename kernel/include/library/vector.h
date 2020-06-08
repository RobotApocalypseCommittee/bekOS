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

#ifndef BEKOS_VECTOR_H
#define BEKOS_VECTOR_H

#include "library/utility.h"
#include "kstring.h"

namespace bek {
    template<class T>
    class vector {
    public:
        vector() : m_size(0), m_capacity(1) {
            array = reinterpret_cast<T*>(kmalloc(m_capacity * sizeof(T)));
        };

        vector(const vector<T> &v) __attribute((deprecated("Do you want to copy vector?"))) {
            m_size = v.m_size;
            m_capacity = v.m_capacity;
            array = reinterpret_cast<T*>(kmalloc(m_capacity * sizeof(T)));
            for (size_t i = 0; i < m_size; i++) {
                new (&array[i]) T(v[i]);
            }
        }

        vector& operator=(const vector<T> &v) __attribute((deprecated("Do you want to copy vector?"))) {
            if (this != v) {
                clear();
                reserve(v.m_size);
                for (size_t i = 0; i < m_size; i++) {
                    new (&array[i]) T(v[i]);
                }
                m_size = v.m_size;
            }
            return *this;
        }

        void reserve(size_t n) {
            while (m_capacity < n) {
                m_capacity <<= 2u;
            }
            reallocate();
        }

        inline void push_back(const T &item) {
            push_back(T(item));
        }

        inline void push_back(T &&item) {
            if (m_capacity == m_size) {
                m_capacity <<= 2u;
                reallocate();
            }
            new (&array[m_size]) T(bek::move(item));
            m_size++;
        }

        inline T &operator[](size_t n) {
            return array[n];
        }

        inline size_t size() {
            return m_size;
        }

        void reallocate() {
            if constexpr (std::is_trivially_copyable_v<T>) {
                array = reinterpret_cast<T*>(krealloc(array, m_capacity * sizeof(T)));
            } else {
                auto new_array = reinterpret_cast<T*>(malloc(array, m_capacity * sizeof(T)));
                for (size_t i = 0; i < m_size; i++) {
                    new (&new_array[i]) T(bek::move(array[i]));
                    array[i].~T();
                }
                kfree(array);
                array = new_array;
            }
        }

        void clear() {
            for (int i = 0; i < m_size; i++) {
                array[i].~T();
            }
            m_size = 0;

        }

        virtual ~vector() {
            clear();
            kfree(array);
        }

    private:
        size_t m_size;
        size_t m_capacity;
        T* array;
    };
}


#endif //BEKOS_VECTOR_H
