/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
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

#ifndef BEKOS_INTRUSIVE_LIST_H
#define BEKOS_INTRUSIVE_LIST_H

#include "bek/assertions.h"
#include "bek/utility.h"
namespace bek::detail {

template <typename T>
class IntrusiveListNode;

template <typename T>
struct IntrusiveListHead {
    IntrusiveListNode<T>* first = nullptr;
    IntrusiveListNode<T>* last  = nullptr;
};

template <typename T>
class IntrusiveListNode {
public:
    template <typename T_, IntrusiveListNode<T_> T_::*MemberPtr>
    friend class IntrusiveList;

    IntrusiveListHead<T>* m_list_head;
    IntrusiveListNode* m_next = nullptr;
    IntrusiveListNode* m_prev = nullptr;

    void remove() {
        if (m_list_head) {
            if (m_list_head->first == this) {
                m_list_head->first = m_next;
            }
            if (m_list_head->last == this) {
                m_list_head->last = m_prev;
            }
            if (m_next) {
                m_next->m_prev = m_prev;
            }
            if (m_prev) {
                m_prev->m_next = m_next;
            }
            m_list_head = nullptr;
            m_next = m_prev = nullptr;
        }
    }
};

template <typename T, IntrusiveListNode<T> T::*MemberPtr>
class IntrusiveList {
    using Node = IntrusiveListNode<T>;

    // --- Iterators ---
    template <bool Forward, bool Const>
    struct Iterator {
        T& operator*()
            requires(!Const)
        {
            return *m_value;
        }
        const T& operator*() const { return *m_value; }
        T* operator->()
            requires(!Const)
        {
            return m_value;
        }
        const T* operator->() const { return m_value; }

        explicit operator bool() const { return m_value; }
        bool operator==(const Iterator& other) const = default;

        Iterator& operator++() {
            if constexpr (Forward) {
                m_value = IntrusiveList::next(m_value);
            } else {
                m_value = IntrusiveList::prev(m_value);
            }
            return *this;
        }
        using Ptr = bek::conditional<Const, const T*, T*>;
        Ptr m_value{nullptr};
    };

public:
    [[nodiscard]] constexpr bool empty() const { return m_head.first == nullptr; }

    void append(T& item) {
        auto& node = item.*MemberPtr;
        ASSERT(!node.m_list_head);
        node.m_list_head = &m_head;
        node.m_prev      = m_head.last;
        node.m_next      = nullptr;
        if (m_head.last) m_head.last->m_next = &node;
        m_head.last      = &node;
        if (!m_head.first) m_head.first = &node;
    }

    void remove(T& item) {
        IntrusiveListNode<T>& node = item.*MemberPtr;
        ASSERT(node.m_list_head == &m_head);
        node.remove();
    }

    void insert_before(T& before, T& insertee) {
        auto& before_node = before.*MemberPtr;
        auto& insertee_node = insertee.*MemberPtr;
        VERIFY(!insertee_node.m_list_head);
        VERIFY(before_node.m_list_head == &m_head);
        insertee_node.m_list_head = &m_head;
        insertee_node.m_next = &before_node;
        insertee_node.m_prev = before_node.m_prev;
        if (before_node.m_prev) {
            before_node.m_prev->m_next = &insertee_node;
        }
        before_node.m_prev = &insertee_node;

        if (m_head.first == &before_node) {
            m_head.first = &insertee_node;
        }
    }

    T& pop_front() {
        VERIFY(m_head.first);
        auto& obj = *object_from_node(m_head.first);
        m_head.first->remove();
        return obj;
    }

    using ForwardIterator = Iterator<true, false>;
    ForwardIterator begin() { return {empty() ? nullptr : &front()}; }
    ForwardIterator end() { return {}; }

    using ReverseIterator = Iterator<false, false>;
    ReverseIterator rbegin() { return {empty() ? nullptr : &back()}; }
    ReverseIterator rend() { return {}; }

    using ConstIterator = Iterator<true, true>;
    ConstIterator begin() const { return {empty() ? nullptr : &front()}; }
    ConstIterator end() const { return {}; }

    T& front() { return *object_from_node(m_head.first); }
    const T& front() const { return *object_from_node(m_head.first); }

    T* front_ptr() { return object_from_node(m_head.first); }

    T& back() { return *object_from_node(m_head.last); }
    const T& back() const { return *object_from_node(m_head.last); }

    uSize size() const {
        uSize s = 0;
        for (auto& item : *this) {
            (void)item;
            s++;
        }
        return s;
    }

private:
    static T* next(const T* current) { return object_from_node((current->*MemberPtr).m_next); }

    static T* prev(const T* current) { return object_from_node((current->*MemberPtr).m_prev); }

    static T* object_from_node(Node* node) {
        static_assert(sizeof(MemberPtr) == sizeof(uPtr));
        if (!node) return nullptr;
        return bek::bit_cast<T*>(bek::bit_cast<char*>(node) - bek::bit_cast<uPtr>(MemberPtr));
    }

    IntrusiveListHead<T> m_head{};
};
}  // namespace bek::detail

namespace bek {
using detail::IntrusiveList;
using detail::IntrusiveListNode;
}  // namespace bek

#endif  // BEKOS_INTRUSIVE_LIST_H
