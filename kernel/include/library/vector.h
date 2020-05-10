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

#include "kstring.h"
#include <utility>
#include "liballoc/liballoc.h"

namespace bek {
    template<class T>
    class vector {
    public:
        vector() : m_size(0), m_capacity(1) {
            array = reinterpret_cast<T*>(kmalloc(m_capacity * sizeof(T)));
        };

        vector(const vector<T> &v) {
            m_size = v.m_size;
            m_capacity = v.m_capacity;
            array = reinterpret_cast<T*>(kmalloc(m_capacity * sizeof(T)));
            memcpy(array, v.array, m_size * sizeof(T));
        }

        void reserve(size_t n) {
            while (m_capacity < n) {
                m_capacity <<= 2u;
            }
            reallocate();
        }

        void push_back(const T &item) {
            if (m_capacity == m_size) {
                m_capacity <<= 2u;
                reallocate();
            }
            array[m_size] = item;
            m_size++;
        }

        void push_back(T &&item) {
            if (m_capacity == m_size) {
                m_capacity <<= 2u;
                reallocate();
            }
            array[m_size] = std::move(item);
            m_size++;
        }

        inline T &operator[](size_t n) {
            return array[n];
        }

        inline size_t size() {
            return m_size;
        }

        void reallocate() {
            array = reinterpret_cast<T*>(krealloc(array, m_capacity * sizeof(T)));
        }

    private:
        size_t m_size;
        size_t m_capacity;
        T* array;
    };
}


#endif //BEKOS_VECTOR_H
