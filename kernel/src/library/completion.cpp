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

#include "library/completion.h"

#include "bek/assertions.h"

#include "library/debug.h"
#include "process/process.h"

using DBG = DebugScope<"Completion", DebugLevel::INFO>;

SingleProcessCompletion::SingleProcessCompletion(Process& proc): m_process(&proc) {}

void SingleProcessCompletion::mark_complete() {
    m_flag = true;
    DBG::infoln("Marking complete for process {}"_sv, m_process->pid());
    // We take lock to (a) order m_flag write/read with other lock acquires (b) prevent lost wake
    ScopeLocker locker{m_lock};
    if (m_process) {
        ProcessManager::the().wake_process(*m_process);
    }
}
void SingleProcessCompletion::wait_on() {
    VERIFY(m_process == &ProcessManager::the().current_process());
    DBG::infoln("Waiting for completion for process {}"_sv, m_process->pid());
    // We obtain the lock whilst we suspend (if necessary) so that we can't miss the wake.
    auto irq_state = m_lock.acquire();
    // We use an atomic load to force a read (don't care about memory ordering)
    while (!__atomic_load_n(&m_flag, __ATOMIC_RELAXED)) {
        ProcessManager::the().suspend_process_and([&]() { m_lock.release(irq_state); });
        // Waking up from sleep without lock
        // Taking lock ensures (ACQUIRE-RELEASE ordering) subsequent read of m_flag correctly ordered;
        // Because _if_ mark_complete is running, either its RELEASED lock -> m_flag propagated
        // Or it hasn't acquired lock yet, and will wake process once it sleeps again
        irq_state = m_lock.acquire();
    }

    // We have lock, and flag is set
    m_lock.release(irq_state);
    DBG::infoln("Finished waiting for completion for process {}"_sv, m_process->pid());
}