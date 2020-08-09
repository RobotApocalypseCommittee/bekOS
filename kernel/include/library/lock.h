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

#ifndef BEKOS_LOCK_H
#define BEKOS_LOCK_H

#include <atomic>

namespace bek {

    class spinlock {
    public:
        spinlock() = default;

        void lock();
        bool try_lock();
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