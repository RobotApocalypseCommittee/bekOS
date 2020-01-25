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

#ifndef BEKOS_ACQUIRABLE_REF_H
#define BEKOS_ACQUIRABLE_REF_H

#include <printf.h>
#include "library/assert.h"

template <class T>
class AcquirableRef {
public:
    explicit AcquirableRef(T* ref);
    AcquirableRef();

    template <class U>
    AcquirableRef(const AcquirableRef<U>& other) noexcept;

    AcquirableRef(AcquirableRef&& other) noexcept;
    AcquirableRef(const AcquirableRef& other);
    AcquirableRef& operator=(AcquirableRef other) noexcept;
    ~AcquirableRef();

    bool operator==(const AcquirableRef& b) const;

    bool unique() const noexcept;
    bool empty() const noexcept;

    void swap(AcquirableRef& lhs) noexcept;

    T* get() const noexcept;
    T& operator*() const;
    T* operator->() const;
private:
    T* m_ref;

    template <class U>
            friend class AcquirableRef;
};

template<class T>
AcquirableRef<T>::AcquirableRef(T* ref): m_ref(ref) {
    if (m_ref != nullptr) {
        m_ref->acquire();
    }
}

template<class T>
AcquirableRef<T>::~AcquirableRef() {
    if (m_ref != nullptr) {
        m_ref->release();
    }
}

template<class T>
AcquirableRef<T>::AcquirableRef(): m_ref(nullptr) {

}

template<class T>
AcquirableRef<T>::AcquirableRef(const AcquirableRef &other): m_ref(other.m_ref) {
    if (m_ref != nullptr) {
        m_ref->acquire();
    }
}

template <class T>
template <class U>
AcquirableRef<T>::AcquirableRef(const AcquirableRef<U>& other): AcquirableRef(static_cast<T*>(other.m_ref)) {}

template<class T>
AcquirableRef<T>& AcquirableRef<T>::operator=(AcquirableRef other) noexcept {
    swap(other);
    return *this;
}

template<class T>
void AcquirableRef<T>::swap(AcquirableRef &lhs) noexcept {
    std::swap(lhs.m_ref, m_ref);
}

template<class T>
T* AcquirableRef<T>::get() const noexcept {
    return m_ref;
}

template<class T>
T &AcquirableRef<T>::operator*() const {
    assert(m_ref != nullptr);
    return *m_ref;
}

template<class T>
T* AcquirableRef<T>::operator->() const {
    assert(m_ref != nullptr);
    return m_ref;
}

template<class T>
AcquirableRef<T>::AcquirableRef(AcquirableRef&& other) noexcept: AcquirableRef() {
    swap(other);
}

template<class T>
bool AcquirableRef<T>::operator==(const AcquirableRef &b) const {
    return b.m_ref == m_ref;
}

template<class T>
bool AcquirableRef<T>::unique() const noexcept {
    if (m_ref != nullptr) {
        if (m_ref->get_ref_count() == 1) {
            return true;
        }
    }
    return false;
}

template<class T>
bool AcquirableRef<T>::empty() const noexcept {
    return m_ref == nullptr;
}

template <class T, class U>
AcquirableRef<T> static_pointer_cast(const AcquirableRef<U>& other) {
    return AcquirableRef<T>(static_cast<T*>(other.get()));
}

#endif //BEKOS_ACQUIRABLE_REF_H
