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

#include "process/process.h"

#include "arch/process_entry.h"
#include "bek/assertions.h"
#include "filesystem/path.h"
#include "interrupts/deferred_calls.h"
#include "interrupts/int_ctrl.h"
#include "library/debug.h"
#include "library/user_buffer.h"
#include "mm/page_allocator.h"
#include "peripherals/timer.h"
#include "process/elf.h"

using DBG = DebugScope<"Process", DebugLevel::WARN>;

constexpr inline uSize KERNEL_STACK_PAGES = 3;
constexpr inline uSize DEFAULT_USER_STACK = 4 * PAGE_SIZE;
constexpr inline uSize MAX_USER_STACK = 1024 * PAGE_SIZE;

constexpr inline uSize CONTEXT_SWITCH_NS = 100'000'00;  // 100ms

Process::Process(bek::string name, Process* parent, mem::VirtualRegion kernel_stack)
    : m_name(bek::move(name)),
      m_pid{-1},
      m_parent{parent},
      m_kernel_stack(kernel_stack),
      m_running_state{ProcessState::Unready} {}

void Process::quit_process(int exit_code) {
    DBG::warnln("Process {} ({}) quit with code {}."_sv, name(), pid(), exit_code);
    m_running_state = ProcessState::AwaitingDeath;
    m_exit_code = exit_code;
    ProcessManager::the().schedule();
    // TODO: What to do if fails.
}
expected<UserBuffer> Process::create_user_buffer(uPtr ptr, uSize size, bool for_writing) {
    if (!m_userspace_state->address_space_manager.check_region(
            ptr, size, for_writing ? MemoryOperation::Write : MemoryOperation::Read)) {
        return EFAULT;
    }
    return UserBuffer(ptr, size);
}

expected<bek::shared_ptr<EntityHandle>> Process::get_open_entity(long entity_id) {
    // TODO: Lock
    if (entity_id < 0) return EBADF;
    if (m_userspace_state->open_entities.size() <= static_cast<uSize>(entity_id)) {
        return EBADF;
    }
    auto entity_ptr = m_userspace_state->open_entities[entity_id].handle;
    if (!entity_ptr) {
        return EBADF;
    }
    return entity_ptr;
}

expected<bek::shared_ptr<Process>> Process::spawn_kernel_process(bek::string name, RawFn fn, void* arg) {
    auto kernel_stack = mem::PageAllocator::the().allocate_region(KERNEL_STACK_PAGES);
    if (!kernel_stack) return ENOMEM;
    auto proc = bek::adopt_shared(new Process(bek::move(name), nullptr, *kernel_stack));
    if (!proc) {
        mem::PageAllocator::the().free_region(kernel_stack->start);
        return ENOMEM;
    }
    // Process is not registered - its PID is invalid, and it is Unready.
    proc->m_saved_registers = SavedRegisters::create_for_kernel_task(fn, arg, kernel_stack->end().get());

    if (auto r = ProcessManager::the().register_process(proc); r != ESUCCESS) {
        // Don't free kernel stack - owned by process now.
        return r;
    }

    return proc;
}

expected<bek::shared_ptr<Process>> Process::spawn_user_process(bek::string name, fs::EntryRef executable,
                                                               fs::EntryRef cwd,
                                                               bek::vector<LocalEntityHandle> handles) {
    auto kernel_stack = mem::PageAllocator::the().allocate_region(KERNEL_STACK_PAGES);
    if (!kernel_stack) return ENOMEM;
    auto proc = bek::adopt_shared(new Process(bek::move(name), nullptr, *kernel_stack));
    if (!proc) {
        mem::PageAllocator::the().free_region(kernel_stack->start);
        return ENOMEM;
    }

    // Don't manually free Kernel Stack from now on - owned by process.

    // This function creates the saved registers structure as well.
    if (auto r = proc->execute_executable(bek::move(executable), bek::move(cwd), bek::move(handles),
                                          bek::vector<bek::string>(), bek::vector<bek::string>());
        r != ESUCCESS) {
        return r;
    }

    if (auto r = ProcessManager::the().register_process(proc); r != ESUCCESS) {
        return r;
    }

    return proc;
}

ErrorCode Process::execute_executable(fs::EntryRef executable, fs::EntryRef cwd, bek::vector<LocalEntityHandle> handles,
                                      bek::vector<bek::string> arguments, bek::vector<bek::string> environ) {
    DBG::dbgln("Trying to execute {}."_sv, executable->name());
    if (has_userspace()) DBG::warnln("Warn: May not support overwriting userspace process."_sv);
    auto name = bek::string{executable->name()};

    auto elf = EXPECTED_TRY(ElfFile::parse_file(bek::move(executable)));
    SpaceManager new_space = EXPECTED_TRY(SpaceManager::create());

    EXPECTED_TRY(elf->load_into(new_space));

    // Now, create stack.
    auto stack_suggestion = elf->get_sensible_stack_region(MAX_USER_STACK);
    mem::UserRegion stack_region{stack_suggestion.end() - DEFAULT_USER_STACK, DEFAULT_USER_STACK};
    auto stack = EXPECTED_TRY(
        new_space.allocate_placed_region(stack_region, MemoryOperation::Read | MemoryOperation::Write, "stack"_sv));

    // Now we need to copy items onto stack.
    auto stack_offset = static_cast<iSize>(stack->size());
    auto stack_bottom_ptr = stack->kernel_mapped_region().start;
    bek::memset(stack->kernel_mapped_region().start.get(), 0, stack->size());

    auto put_string_on_stack = [&](const bek::string& str) {
        auto needed_size = str.size() + 1;
        if (stack_offset < needed_size) {
            DBG::errln("Error: No space on user stack to put string \"{}\"."_sv, str.view());
            return ENOMEM;
        } else {
            stack_offset -= needed_size;
            bek::memcopy(stack_bottom_ptr.offset(stack_offset).ptr, str.data(), needed_size);
            return ESUCCESS;
        }
    };

    bek::vector<mem::UserPtr> init_stack_ptr_array;
    init_stack_ptr_array.reserve(arguments.size() + environ.size() + 2);

    for (const auto& arg : arguments) {
        EXPECT_SUCCESS(put_string_on_stack(arg));
        init_stack_ptr_array.push_back(stack_region.start.offset(stack_offset));
    }
    init_stack_ptr_array.push_back(mem::UserPtr{});
    for (const auto& env : environ) {
        EXPECT_SUCCESS(put_string_on_stack(env));
        init_stack_ptr_array.push_back(stack_region.start.offset(stack_offset));
    }
    init_stack_ptr_array.push_back(mem::UserPtr{});

    auto needed_size = init_stack_ptr_array.size() * sizeof(uPtr);
    // FIXME: Stack pointer alignment should be arch-dependent constant.
    needed_size += stack_offset % 16;
    if (stack_offset < needed_size) {
        DBG::errln("Error: No space on user stack to put the init array."_sv);
        return ENOMEM;
    }
    stack_offset -= needed_size;
    bek::memcopy(stack_bottom_ptr.offset(stack_offset).ptr, init_stack_ptr_array.data(),
                 init_stack_ptr_array.size() * sizeof(uPtr));

    m_userspace_state = UserspaceState{stack_region.start.offset(stack_offset), bek::move(cwd), bek::move(new_space),
                                       bek::move(handles)};
    m_name = bek::move(name);

    DBG::dbgln("Executing process {}. Address space:"_sv, this->name());
    m_userspace_state->address_space_manager.debug_print();

    if (&ProcessManager::the().current_process() == this) {
        ProcessManager::the().enter_critical();
    }
    m_saved_registers = SavedRegisters::create_for_user_execute(elf->get_entry_point().ptr, m_kernel_stack.end().get(),
                                                                stack_region.start.offset(stack_offset).ptr);
    return ESUCCESS;
}

long Process::allocate_entity_handle_slot(bek::shared_ptr<EntityHandle> handle, u8 group) {
    // TODO: Lock
    VERIFY(m_userspace_state);
    m_userspace_state->open_entities.push_back({bek::move(handle), group});
    return m_userspace_state->open_entities.size() - 1;
}

// region ProcessManager

ProcessManager* g_process_manager{nullptr};

ErrorCode ProcessManager::initialise_and_adopt(bek::string name, mem::VirtualRegion kernel_stack) {
    VERIFY(!g_process_manager);
    g_process_manager = new ProcessManager();
    if (!g_process_manager) return ENOMEM;

    auto proc = bek::adopt_shared(new Process(bek::move(name), nullptr, kernel_stack));
    if (!proc) return ENOMEM;
    proc->m_saved_registers = {};
    proc->m_pid = 0;
    proc->m_running_state = ProcessState::Running;

    return g_process_manager->initialise_with_scheduling(proc);
}

ErrorCode ProcessManager::initialise_with_scheduling(bek::shared_ptr<Process> first_process) {
    VERIFY(m_processes.size() == 0 && m_current == nullptr);
    m_last_nanoseconds = timing::nanoseconds_since_start();
    first_process->m_pid = 0;
    m_current = first_process.get();
    m_processes.push_back(bek::move(first_process));

    // Now start scheduler.
    return timing::schedule_callback(
        [](u64) {
            VERIFY(deferred::queue_call([]() { g_process_manager->schedule(); }) == ESUCCESS);
            return TimerDevice::CallbackAction::Reschedule(CONTEXT_SWITCH_NS);
        },
        CONTEXT_SWITCH_NS);
}

ProcessManager& ProcessManager::the() {
    VERIFY(g_process_manager);
    return *g_process_manager;
}

Process& ProcessManager::current_process() { return *m_current; }

void ProcessManager::enter_critical() {
    // Don't want process changing halfway through!
    InterruptDisabler disabler;
    m_current->m_preempt_counter++;
}

void ProcessManager::exit_critical() {
    InterruptDisabler disabler;
    m_current->m_preempt_counter--;
}
bool ProcessManager::is_critical() const {
    InterruptDisabler d;
    return m_current->m_preempt_counter;
}

int ProcessManager::count_critical() const {
    InterruptDisabler d;
    return m_current->m_preempt_counter;
}

ErrorCode ProcessManager::register_process(bek::shared_ptr<Process> proc) {
    enter_critical();
    auto& proc_ref = *proc;
    if (proc_ref.pid() >= 0) {
        // Merely check the PID.
        if ((proc_ref.pid() < m_processes.size() && m_processes[proc_ref.pid()]) ||
            proc_ref.pid() > m_processes.size()) {
            return EINVAL;
        }
    } else {
        uSize candidate = m_processes.size();
        for (uSize i = 0; i < m_processes.size(); i++) {
            if (!m_processes[i]) {
                candidate = i;
                break;
            }
        }

        // Unlikely!
        if (candidate >= bek::numeric_limits<long>::max()) {
            return ENOMEM;
        }
        proc_ref.m_pid = static_cast<long>(candidate);
    }
    if (proc_ref.pid() < m_processes.size()) {
        m_processes[proc_ref.pid()] = bek::move(proc);
    } else {
        VERIFY(proc_ref.pid() == m_processes.size());
        m_processes.push_back(bek::move(proc));
    }
    exit_critical();
    if (proc_ref.m_running_state == ProcessState::Unready) {
        proc_ref.m_running_state = ProcessState::Stopped;
    }
    return ESUCCESS;
}
bool ProcessManager::schedule() {
    enter_critical();
    if (count_critical() != 1) {
        // Already in critical section.
        exit_critical();
        return false;
    }
    // We are allowed to schedule
    // Try to avoid getting scheduled back...
    m_current->m_processor_time_counter = 0;
    Process* best_process = nullptr;
    while (true) {
        long max_counter = -1;
        best_process = nullptr;
        for (auto& process : m_processes) {
            if (process && process->m_running_state == ProcessState::Running &&
                process->m_processor_time_counter > max_counter) {
                max_counter = process->m_processor_time_counter;
                best_process = process.get();
            }
        }

        // There is a process owed ticks
        if (max_counter > 0) {
            // Give it a tick
            VERIFY(best_process);
            break;
        } else {
            // We need to increment ticks of the processes:
            for (auto& process : m_processes) {
                if (process) {
                    process->m_processor_time_counter++;
                }
            }
        }
    }
    // We have chosen the next process
    auto cur_nanoseconds = timing::nanoseconds_since_start();
    auto last_nanoseconds = bek::exchange(m_last_nanoseconds, cur_nanoseconds);
    auto ms_duration = (cur_nanoseconds - last_nanoseconds) / 1'000'000;
    DBG::dbgln("Switch to: {} ({}); cycle took {}ms."_sv, best_process->name(), best_process->pid(), ms_duration);
    switch_context(*best_process);
    exit_critical();
    return true;
}
void ProcessManager::switch_context(Process& process) {
    // Exclusive critical section.
    InterruptDisabler disabler;
    VERIFY(count_critical() == 1);
    if (&process == m_current) {
        // Do nothing
        return;
    }

    SavedRegisters& previous_registers = m_current->m_saved_registers;
    m_current = &process;

    if (m_current->has_userspace()) {
        do_switch_user_address_space(m_current->m_userspace_state->address_space_manager.raw_root_ptr());
    }
    // Perform the switch - who knows when this function will return?
    do_context_switch(previous_registers, m_current->m_saved_registers);
}
ErrorCode ProcessManager::reap_process(Process& proc) {
    VERIFY(m_processes[proc.pid()].get() == &proc);
    VERIFY(proc.m_children.size() == 0);
    VERIFY(proc.ref_count() == 1);
    m_processes[proc.pid()] = nullptr;
    return ESUCCESS;
}

// endregion
