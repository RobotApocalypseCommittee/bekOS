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

#ifndef BEKOS_LOCK_H
#define BEKOS_LOCK_H

#include <atomic>

#include "bek/types.h"

namespace bek {

    class spinlock {
        using locktype = u32;
    public:
        spinlock() = default;

        /// Acquires lock; will block until it succeeds (can cause deadlock).
        void lock();

        /// Try to acquire lock, does not block if it is already locked. If true returned, *must* unlock later.
        bool try_lock();

        /// Unlocks a mutex - must be in thread which has locked it already, or warranty void.
        void unlock();

        spinlock(const spinlock&) = delete;
        spinlock& operator=(const spinlock&) = delete;
        spinlock(spinlock&&) = delete;
        spinlock& operator=(spinlock&&) = delete;
    private:
        std::atomic_flag state = ATOMIC_FLAG_INIT;
    };

    template <class T>
    class locker {
    public:
        explicit locker(T& lock): lock(lock) {
            lock.lock();
        }

        locker(const locker&) = delete;
        locker& operator=(const locker&) = delete;

        ~locker() {
            lock.unlock();
        }

    private:
        T& lock;
    };

}

#endif //BEKOS_LOCK_H
