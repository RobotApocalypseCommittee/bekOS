/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2026 Bekos Contributors
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


#ifndef BEKOS_WAITQUEUE_H
#define BEKOS_WAITQUEUE_H
#include "function.h"
#include "intrusive_list.h"
#include "locking.h"

class WaitQueue {
public:
    enum class WakeReason: char {
        Wake,
        Cancel,
    };

    struct WaitItem {
        bek::function<void(WakeReason)> function;
        bek::IntrusiveListNode<WaitItem> list_node;
        using List = bek::IntrusiveList<WaitItem, &WaitItem::list_node>;
        explicit WaitItem(bek::function<void(WakeReason)> function): function(bek::move(function)) {}
    };

    void add(WaitItem& item);
    void remove(WaitItem& item);
    void wake_one();
    void wake_all();
    void cancel_all();

private:
    WaitItem::List m_list{};
    SpinLock m_lock;
};

#endif //BEKOS_WAITQUEUE_H
