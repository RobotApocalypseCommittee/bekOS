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

#include "library/sleeplock.h"

#include "process/process.h"

void SleepLock::acquire() {
    // First, we try to obtain the lock (fast path)
    if (try_acquire_quick()) return;

    auto irq_state = m_queue_lock.acquire();
    auto& wq_item = ProcessManager::the().current_process().sleeplock_wq_item();
    // Add to waitqueue; ready to be woken.
    m_queue.append(wq_item);

    bool acquired = false;
    // We need to loop because once we're woken, some new process may have grabbed the lock via fast path
    while (!acquired) {
        // Optimistic: now we have the lock, an unlock operation may have finished (ordering).
        if (try_acquire_quick()) {
            acquired = true;
            break;
        }

        // If not, release the lock and sleep (woken by release())
        // We need to release the lock _after_ sleeping so that wake is guaranteed to succeed.
        ProcessManager::the().suspend_process_and([&]() { m_queue_lock.release(irq_state); });

        // We're back: ASAP try to get lock
        acquired = try_acquire_quick();

        // Regardless, we need the lock
        irq_state = m_queue_lock.acquire();
    }
    // At this point, we (a) have the sleeplock, (b) have the queue lock, and (c) are in the wq
    VERIFY(acquired);
    m_queue.remove(wq_item);
    m_queue_lock.release(irq_state);
}

void SleepLock::release() {
    VERIFY(m_owner == &ProcessManager::the().current_process());
    // Simple: all we do is first, release the lock, then obtain the queue and wake the first waiter
    __atomic_store_n(&m_owner, nullptr, __ATOMIC_RELEASE);
    ScopeLocker locker{m_queue_lock};
    if (!m_queue.empty()) {
        // It's the woken process's responsibility to remove itself from queue when it has acquired lock
        Process* process_to_wake = m_queue.front().process;
        ProcessManager::the().wake_process(*process_to_wake);
    }
}

bool SleepLock::try_acquire_quick() {
    Process* old = nullptr;
    Process* new_owner = &ProcessManager::the().current_process();
    return __atomic_compare_exchange_n(&m_owner, &old, new_owner, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}