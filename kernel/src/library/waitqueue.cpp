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

#include "library/waitqueue.h"

void WaitQueue::add(WaitItem& item) {
    ScopeLocker locker{m_lock};
    m_list.append(item);
}
void WaitQueue::remove(WaitItem& item) {
    ScopeLocker locker{m_lock};
    m_list.remove(item);
}
void WaitQueue::wake_one() {
    auto& item = with_lock(m_lock, [this]() -> WaitItem& {
        return m_list.pop_front();
    });
    item.function(WakeReason::Wake);
}
void WaitQueue::wake_all() {
    ScopeLocker locker{m_lock};
    while (!m_list.empty()) {
        auto& item = m_list.pop_front();
        item.function(WakeReason::Wake);
    }
}
void WaitQueue::cancel_all() {
    ScopeLocker locker{m_lock};
    while (!m_list.empty()) {
        auto& item = m_list.pop_front();
        item.function(WakeReason::Cancel);
    }
}