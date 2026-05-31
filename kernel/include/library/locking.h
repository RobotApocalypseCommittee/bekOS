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

#ifndef BEKOS_LOCKING_H
#define BEKOS_LOCKING_H
#include "bek/types.h"

class Process;

/**
 * @class SpinLock
 * @brief spinlock which disables preemption (not irqs)
 * @warning usafe for use in interrupt handlers
 */
struct SpinLock {
    void acquire();
    void release();

private:
    bool locked = false;
};

/**
 * @class IrqSpinLock
 * @brief spinlock which disables preemption and irqs
 */
struct IrqSpinLock {
    using irq_state_t = u8;
    [[nodiscard]] irq_state_t acquire();
    void release(irq_state_t irq_state);

private:
    bool locked = false;
};

template <typename T>
class ScopeLocker {
    T& m_lock;

public:
    explicit ScopeLocker(T& lock): m_lock(lock) { m_lock.acquire(); }
    ~ScopeLocker() { m_lock.release(); }
};

template <>
class ScopeLocker<IrqSpinLock> {
    IrqSpinLock& m_lock;
    IrqSpinLock::irq_state_t m_irq_state;

public:
    explicit ScopeLocker(IrqSpinLock& lock): m_lock(lock), m_irq_state(m_lock.acquire()) {}
    ~ScopeLocker() { m_lock.release(m_irq_state); }
};

template <typename L, typename F>
auto with_lock(L& lock, F&& f) -> decltype(f()) {
    ScopeLocker<L> locker{lock};
    return f();
}

#endif  // BEKOS_LOCKING_H
