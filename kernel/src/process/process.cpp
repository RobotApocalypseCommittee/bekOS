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

#include <memory_manager.h>
#include <utils.h>
#include <library/assert.h>
#include <printf.h>
#include "process/process.h"
#include "process/process_entry.h"

extern memory_manager memoryManager;

Process* ProcessManager::getCurrentProcess() {
    return m_current;
}

void ProcessManager::fork(ProcessFn fn, u64 argument) {
    // Don't schedule while adding a process
    disablePreempt();
    // In pages
    auto* p = reinterpret_cast<Process*>(phys_to_virt(
            memoryManager.reserve_region(KERNEL_STACK_SIZE / 4096, PAGE_KERNEL)));
    p->processorTimeCounter = 1;

    // Nothing prevents it from being switched away from, even from the beginning
    p->preemptCounter = 0;
    p->state = ProcessState::RUNNING;
    p->processID = -1;
    for (int i = 0; i < m_processes.size(); i++) {
        if (m_processes[i] == nullptr) {
            p->processID = i;
            break;
        }
    }
    if (p->processID == -1) {
        p->processID = m_processes.size();
        m_processes.push_back(p);
    }
    // The code which will run the function fn
    p->savedRegs.pc = reinterpret_cast<u64>(process_begin);
    // The function, and its argument
    p->savedRegs.x19 = reinterpret_cast<u64>(fn);
    p->savedRegs.x20 = argument;
    // Stack moves downwards
    p->savedRegs.sp = reinterpret_cast<u64>(p) + KERNEL_STACK_SIZE;
    // Done
    enablePreempt();
}

void ProcessManager::disablePreempt() {
    m_current->preemptCounter++;
}

void ProcessManager::enablePreempt() {
    m_current->preemptCounter--;
}

ProcessManager::ProcessManager() {
    // This is initialised in the 'init' process - add this to the list
    auto* p = new Process;
    if (p != nullptr) {
        p->preemptCounter = 0;
        p->processID = 0;
        p->state = ProcessState::RUNNING;
        p->processorTimeCounter = 1;
        m_current = p;
        m_processes.push_back(p);
    } else {
        assert(0);
    }
}

bool ProcessManager::schedule() {
    if (m_current->preemptCounter == 0) {
        // We are allowed to schedule
        // Try to avoid getting scheduled back...
        m_current->processorTimeCounter = 0;

        // Cannot allow scheduler to run twice...
        disablePreempt();
        int maxCounter;
        int bestProcessID;
        while(1) {
            maxCounter = -1;
            bestProcessID = 0;
            for (int i = 0; i < m_processes.size(); i++) {
                if (m_processes[i] && m_processes[i]->state == ProcessState::RUNNING &&
                    m_processes[i]->processorTimeCounter > maxCounter) {
                    maxCounter = m_processes[i]->processorTimeCounter;
                    bestProcessID = i;
                }
            }
            // There is a process owed ticks
            if (maxCounter > 0) {
                // Give it a tick
                break;
            } else {
                // We need to increment ticks of the processes:
                for (int i = 0; i < m_processes.size(); i++) {
                    if (m_processes[i]) {
                        m_processes[i]->processorTimeCounter++;
                    }
                }
            }
        }
        // We have chosen the next process
        //printf("Switch: %d\n", bestProcessID);
        switchContext(m_processes[bestProcessID]);
        enablePreempt();
        return true;
    } else {
        return false;
    }
}

void ProcessManager::switchContext(Process* process) {
    if (process == m_current) {
        // Do nothing
        return;
    }
    SavedRegs* previousRegisters = &m_current->savedRegs;
    m_current = process;
    // Perform the switch - who knows when this function will return?
    do_context_switch(previousRegisters, &m_current->savedRegs);
}
