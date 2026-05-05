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

#ifndef BEKOS_SLEEPLOCK_H
#define BEKOS_SLEEPLOCK_H

#include "intrusive_list.h"
#include "locking.h"

class Process;

class SleepLock {
public:
    struct QueuedProcess {
        explicit QueuedProcess(Process* process): process(process) {}
        Process* process;
        bek::IntrusiveListNode<QueuedProcess> list_node;
    };

    void acquire();
    void release();

private:
    bool try_acquire_quick();

    Process* m_owner{nullptr};
    // Use IRQ lock just in case (not technically needed as mutex cannot be used from IRQ)
    IrqSpinLock m_queue_lock;
    bek::IntrusiveList<QueuedProcess, &QueuedProcess::list_node> m_queue;
};

#endif  // BEKOS_SLEEPLOCK_H
