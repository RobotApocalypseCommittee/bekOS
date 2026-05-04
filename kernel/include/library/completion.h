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

#ifndef BEKOS_COMPLETION_H
#define BEKOS_COMPLETION_H
#include "locking.h"

class Process;

class SingleProcessCompletion {
public:
    explicit SingleProcessCompletion(Process& proc);
    void mark_complete();
    void wait_on();

private:
    Process* m_process;
    // Need Irq because mark_complete may be called in interrupt context.
    IrqSpinLock m_lock;
    bool m_flag{false};
};

#endif  // BEKOS_COMPLETION_H
