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

#ifndef BEKOS_PROCESS_H
#define BEKOS_PROCESS_H

#include <bek/vector.h>
#include <filesystem/filesystem.h>

#include "api/syscalls.h"
#include "arch/saved_registers.h"
#include "entity.h"
#include "library/function.h"
#include "library/user_buffer.h"
#include "mm/space_manager.h"
#include "peripherals/device.h"

enum class ProcessState {
    Unready,
    Stopped,
    Running,
    Waiting,
    AwaitingDeath,
};

expected<long> handle_syscall(u64 syscall_no, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6, u64 arg7,
                              InterruptContext& ctx);

class Process : public bek::RefCounted<Process> {
public:
    struct LocalEntityHandle {
        bek::shared_ptr<EntityHandle> handle;
        u8 group;
    };
    using RawFn = void (*)(void*);

    bek::str_view name() const { return m_name.view(); }
    long pid() const { return m_pid; }
    ProcessState set_state(ProcessState new_state) {
        auto old_state = m_running_state;
        m_running_state = new_state;
        return old_state;
    }

    bool has_userspace() const { return m_userspace_state.is_valid(); }

    void quit_process(int exit_code);

    expected<long> sys_open(uPtr path_str, uSize path_len, sc::OpenFlags flags, int parent, uPtr stat_struct);
    expected<long> sys_read(int entity_handle, uSize offset, uPtr buffer, uSize len);
    expected<long> sys_write(int entity_handle, uSize offset, uPtr buffer, uSize len);
    expected<long> sys_seek(int entity_handle, sc::SeekLocation location, iSize offset);
    expected<long> sys_close(int entity_handle);
    expected<long> sys_stat(int entity_handle, uPtr path_str, uSize path_len, bool follow_symlinks, uPtr stat_struct);
    expected<long> sys_get_directory_entries(int entity_handle, uSize offset, uPtr buffer, uSize len);
    expected<long> sys_list_devices(uPtr buffer, uSize len, u64 protocol_filter);
    expected<long> sys_allocate(uPtr address, uSize size, sc::AllocateFlags flags);
    expected<long> sys_deallocate(uPtr address, uSize size);
    expected<long> sys_get_pid();
    expected<long> sys_open_device(uPtr path_str, uPtr path_len);
    expected<long> sys_message_device(int entity_handle, u64 id, uPtr buffer, uSize size);
    expected<long> sys_fork(InterruptContext& ctx);
    expected<long> sys_execute(uPtr executable_path, uSize path_len, uPtr args_array, uSize args_n);
    expected<long> sys_create_pipe(uPtr pipe_handle_arr, u64 raw_flags);
    expected<long> sys_duplicate(long handle_slot, long new_handle_slot, u8 group);

    template <typename Fn>
    auto with_space_manager(Fn&& fn) {
        VERIFY(has_userspace());
        return fn(m_userspace_state->address_space_manager);
    }

    expected<UserBuffer> create_user_buffer(uPtr ptr, uSize size, bool for_writing);
    static expected<bek::shared_ptr<Process>> spawn_kernel_process(bek::string name, RawFn fn, void* arg);
    static expected<bek::shared_ptr<Process>> spawn_user_process(bek::string name, fs::EntryRef executable,
                                                                 fs::EntryRef cwd,
                                                                 bek::vector<LocalEntityHandle> handles);

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&) = delete;
    Process& operator=(Process&&) = delete;

private:
    Process(bek::string name, Process* parent, mem::VirtualRegion kernel_stack);
    expected<bek::shared_ptr<EntityHandle>> get_open_entity(long entity_id);
    [[nodiscard]] ErrorCode execute_executable(fs::EntryRef executable, fs::EntryRef cwd,
                                               bek::vector<LocalEntityHandle> handles);

    long allocate_entity_handle_slot(bek::shared_ptr<EntityHandle> handle, u8 group);

    // Basic properties.
    bek::string m_name;
    long m_pid;

    // Child and Parent relations
    Process* m_parent;
    bek::vector<Process*> m_children;

    // Important State
    SavedRegisters m_saved_registers{};
    mem::VirtualRegion m_kernel_stack{};

    struct UserspaceState {
        mem::UserPtr user_stack_top;
        fs::EntryRef cwd;
        SpaceManager address_space_manager;
        bek::vector<LocalEntityHandle> open_entities;
    };
    // TODO: Lock these.
    bek::optional<UserspaceState> m_userspace_state;

    // Scheduling Information
    long m_processor_time_counter{1};
    int m_preempt_counter{0};
    ProcessState m_running_state;

    // Other Information
    bek::optional<int> m_exit_code;

    friend class ProcessManager;
};

class ProcessManager {
public:
    static ErrorCode initialise_and_adopt(bek::string name, mem::VirtualRegion kernel_stack);
    static ProcessManager& the();

    /// Inserts process into process table, allocating PID if necessary.
    ErrorCode register_process(bek::shared_ptr<Process> proc);

    void enter_critical();
    void exit_critical();
    bool is_critical() const;
    int count_critical() const;

    bool schedule();

    Process& current_process();

    ProcessManager(const ProcessManager&) = delete;

    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

private:
    ProcessManager() = default;
    ErrorCode initialise_with_scheduling(bek::shared_ptr<Process> first_process);
    void switch_context(Process& process);

    bek::vector<bek::shared_ptr<Process>> m_processes{};
    Process* m_current{nullptr};
    uSize m_last_nanoseconds;
};

#endif  // BEKOS_PROCESS_H
