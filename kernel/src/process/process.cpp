/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#include "process/process.h"

#include "bek/assertions.h"
#include "filesystem/path.h"
#include "interrupts/int_ctrl.h"
#include "library/debug.h"
#include "library/user_buffer.h"
#include "mm.h"
#include "mm/page_allocator.h"
#include "process/process_entry.h"

using DBG = DebugScope<"Process", true>;

constexpr inline uSize KERNEL_STACK_PAGES = 2;

Process& ProcessManager::current_process() { return *m_current; }

void ProcessManager::disablePreempt() {
    // Don't want process changing halfway through!
    InterruptDisabler disabler;
    m_current->preemptCounter++;
}

void ProcessManager::enablePreempt() {
    InterruptDisabler disabler;
    m_current->preemptCounter--;
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
        while (true) {
            maxCounter = -1;
            bestProcessID = 0;
            for (uSize i = 0; i < m_processes.size(); i++) {
                if (m_processes[i] && m_processes[i]->state == ProcessState::Running &&
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
                for (uSize i = 0; i < m_processes.size(); i++) {
                    if (m_processes[i]) {
                        m_processes[i]->processorTimeCounter++;
                    }
                }
            }
        }
        // We have chosen the next process
        DBG::dbgln("Switch to: {} ({})."_sv, m_processes[bestProcessID]->name(), bestProcessID);
        switchContext(*m_processes[bestProcessID]);
        enablePreempt();
        return true;
    } else {
        return false;
    }
}

void ProcessManager::switchContext(Process& process) {
    InterruptDisabler disabler;
    if (&process == m_current) {
        // Do nothing
        return;
    }
    SavedRegs* previousRegisters = &m_current->saved_regs;
    m_current                    = &process;

    // Switch userspace mem-mapping
    set_usertable(m_current->m_userspace_state->address_space_manager.raw_root_ptr());
    // Perform the switch - who knows when this function will return?
    do_context_switch(previousRegisters, &m_current->saved_regs);
}

ProcessManager* g_process_manager{nullptr};

ProcessManager& ProcessManager::the() {
    VERIFY(g_process_manager);
    return *g_process_manager;
}
Process& ProcessManager::create_kernel_process(bek::string name, void (*init_fn)(void*),
                                               void* data) {
    disablePreempt();
    int pid = m_processes.size();
    for (uSize i = 0; i < m_processes.size(); i++) {
        if (!m_processes[i]) {
            pid = i;
            break;
        }
    }
    auto process = Process::create_kernel_process(pid, bek::move(name), init_fn, data);
    if (pid == m_processes.size()) {
        m_processes.push_back(bek::move(process));
    } else {
        bek::swap(m_processes[pid], process);
    }
    enablePreempt();
    return *m_processes[pid];
}

bek::own_ptr<Process> Process::create_kernel_process(int pid, bek::string name,
                                                     void (*init_fn)(void*), void* data) {
    auto kernel_stack = mem::PageAllocator::the().allocate_region(KERNEL_STACK_PAGES);
    if (!kernel_stack) return nullptr;
    auto ptr = bek::own_ptr{new Process(pid, bek::move(name), {})};
    if (!ptr) return ptr;
    ptr->kernel_stack   = *kernel_stack;
    ptr->state          = ProcessState::Running;
    ptr->saved_regs.sp  = kernel_stack->end().raw();
    ptr->saved_regs.pc  = reinterpret_cast<u64>(process_begin);
    ptr->saved_regs.x19 = reinterpret_cast<u64>(init_fn);
    ptr->saved_regs.x20 = reinterpret_cast<u64>(data);
    return ptr;
}
Process::Process(int pid, bek::string name, bek::optional<SpaceManager> userspace_manager)
    : m_name{bek::move(name)}, m_pid{pid}, m_userspace_manager(bek::move(userspace_manager)) {}
void Process::quit_process(int exit_code) {
    // TODO: Exit Status
    DBG::dbgln("Process {} ({}) quit with code {}."_sv, name(), pid(), exit_code);
    state = ProcessState::AwaitingDeath;
    ProcessManager::the().schedule();
    // TODO: WHat to do if failes.
}

expected<UserBuffer> Process::create_user_buffer(uPtr ptr, uSize size, bool for_writing) {
    if (!m_userspace_state->address_space_manager.check_region(
            ptr, size, for_writing ? MemoryOperation::Write : MemoryOperation::Read)) {
        return EFAULT;
    }
    return UserBuffer(ptr, size);
}
expected<bek::shared_ptr<EntityHandle>> Process::get_open_entity(int entity_id) {
    // TODO: Lock
    if (entity_id < 0) return EBADF;
    if (m_userspace_state->open_entities.size() <= static_cast<uSize>(entity_id)) {
        return EBADF;
    }
    auto entity_ptr = m_userspace_state->open_entities[entity_id];
    if (!entity_ptr) {
        return EBADF;
    }
    return entity_ptr;
}
