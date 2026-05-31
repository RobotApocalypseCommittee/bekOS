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

#include "library/locking.h"

#include "interrupts/int_ctrl.h"
#include "library/debug.h"
#include "process/process.h"

using DBG = DebugScope<"SpinLock", DebugLevel::INFO>;

void SpinLock::acquire() {
    ProcessManager::the().enter_critical();
    while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
    }
}
void SpinLock::release() {
    __atomic_clear(&locked, __ATOMIC_RELEASE);
    ProcessManager::the().exit_critical();
}
IrqSpinLock::irq_state_t IrqSpinLock::acquire() {
    auto state = save_and_disable_interrupts();
    // NB: don't need to enter critical because interrupts are off.
    while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
    }
    DBG::infoln("SpinLock held"_sv);
    return state;
}
void IrqSpinLock::release(irq_state_t irq_state) {
    __atomic_clear(&locked, __ATOMIC_RELEASE);
    DBG::infoln("SpinLock released"_sv);
    restore_interrupts(irq_state);
}