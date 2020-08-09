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

#ifndef BEKOS_OPTIONAL_H
#define BEKOS_OPTIONAL_H

namespace bek {
    template<typename T>
    class alignas(T) optional {
    public:
        optional(): m_valid(false) {};
        optional(T&& object): m_valid(true) {
            new (&m_data[0]) T(move(object));
        }

        optional(optional&& other): m_valid(other.m_valid) {
            if (m_valid) {
                new (&m_data[0]) T(move(other.value()));
                other.value().~T();
            }
            other.m_valid = false;
        }

        optional& operator=(optional&& other) {
            if (m_valid) value().~T();
            if (m_valid) {
                new (&m_data[0]) T(move(other.value()));
                other.value().~T();
            }
            other.m_valid = false;
            return *this;
        }

        T& value() {
            assert(m_valid);
            return *reinterpret_cast<T*>(&m_data[0]);
        }
        const T& value() const {
            assert(m_valid);
            return *reinterpret_cast<T*>(&m_data[0]);
        }

        T release() {
            assert(m_valid);
            T released = value();
            value().~T();
            return released;
        }

        bool is_valid() const {
            return m_valid;
        }

        ~optional() {
            if (m_valid) value().~T();
        }

    private:
        alignas(T) u8 m_data[sizeof(T)];
        bool m_valid;
    };
}

#endif //BEKOS_OPTIONAL_H
