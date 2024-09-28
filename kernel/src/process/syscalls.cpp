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

#include "api/syscalls.h"

#include <bek/types.h>
#include <library/kernel_error.h>

#include "arch/process_entry.h"
#include "interrupts/int_ctrl.h"
#include "library/debug.h"
#include "mm/page_allocator.h"
#include "peripherals/timer.h"
#include "process/pipe.h"
#include "process/process.h"

using DBG = DebugScope<"Process", DebugLevel::WARN>;

expected<long> handle_syscall(u64 syscall_no, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6, u64 arg7,
                              InterruptContext& ctx) {
    // Takes from registers w0, x1-x7.
    sc::SysCall syscall{syscall_no};
    Process& current_process = ProcessManager::the().current_process();
    VERIFY(current_process.has_userspace());
    switch (syscall) {
        case sc::SysCall::Open:
            return current_process.sys_open(arg1, arg2, static_cast<sc::OpenFlags>(arg3), arg4, arg5);
        case sc::SysCall::Close:
            return current_process.sys_close(arg1);
        case sc::SysCall::Read:
            return current_process.sys_read(arg1, arg2, arg3, arg4);
        case sc::SysCall::Write:
            return current_process.sys_write(arg1, arg2, arg3, arg4);
        case sc::SysCall::Seek:
            return current_process.sys_seek(arg1, static_cast<sc::SeekLocation>(arg2), arg3);
        case sc::SysCall::GetDirEntries:
            return current_process.sys_get_directory_entries(arg1, arg2, arg3, arg4);
        case sc::SysCall::Stat:
            return current_process.sys_stat(arg1, arg2, arg3, arg4, arg5);
        case sc::SysCall::ListDevices:
            return current_process.sys_list_devices(arg1, arg2, arg3);
        case sc::SysCall::OpenDevice:
            return current_process.sys_open_device(arg1, arg2);
        case sc::SysCall::CommandDevice:
            return current_process.sys_message_device(arg1, arg2, arg3, arg4);
        case sc::SysCall::Allocate:
            return current_process.sys_allocate(arg1, arg2, static_cast<sc::AllocateFlags>(arg3));
        case sc::SysCall::Deallocate:
            return current_process.sys_deallocate(arg1, arg2);
        case sc::SysCall::GetPid:
            return current_process.sys_get_pid();
        case sc::SysCall::Fork:
            return current_process.sys_fork(ctx);
        case sc::SysCall::Sleep:
            timing::spindelay_us(arg1);
            return 0;
        case sc::SysCall::Exec:
            return current_process.sys_execute(arg1, arg2, arg3, arg4, arg5, arg6);
        case sc::SysCall::CreatePipe:
            return current_process.sys_create_pipe(arg1, arg2);
        case sc::SysCall::Duplicate:
            return current_process.sys_duplicate(arg1, arg2, arg3);
        case sc::SysCall::Wait:
            return current_process.sys_wait(arg1, arg2, arg3);
        case sc::SysCall::ChangeWorkingDirectory:
            return current_process.sys_chdir(arg1, arg2);
        case sc::SysCall::Exit:
            current_process.quit_process(arg1);
            ASSERT_UNREACHABLE();
            break;
        default:
            return ENOTSUP;
    }
}

expected<long> Process::sys_open(uPtr path_str, uSize path_len, sc::OpenFlags flags, int parent, uPtr stat_struct) {
    using namespace fs;
    // Caution - path needs stable reference to path string.
    auto path_string = EXPECTED_TRY(read_string_from_user(path_str, path_len));
    auto the_path = EXPECTED_TRY(path::parse_path(path_string));

    // TODO: Lock userspace state.
    EntryRef root = m_userspace_state->cwd;
    if (parent != sc::INVALID_ENTITY_ID) {
        auto entity = EXPECTED_TRY(get_open_entity(parent));
        if (auto* file_entity = EntityHandle::as<FileHandle>(*entity); file_entity) {
            root = EntryRef{&file_entity->entry()};
        } else {
            // Not a valid target.
            return ENOTDIR;
        }
    }

    EntryRef parent_holder{};
    auto result = fullPathLookup(root, the_path, &parent_holder);

    if (result.has_error() && parent_holder &&
        (flags & (sc::OpenFlags::CreateIfMissing | sc::OpenFlags::CreateOnly)) != sc::OpenFlags::None) {
        // Didn't find, but will create.
        auto f_name = the_path.segments()[the_path.segments().size()];

        result = EXPECTED_TRY(
            parent_holder->add_child(f_name, (flags & sc::OpenFlags::DIRECTORY) == sc::OpenFlags::DIRECTORY));
    } else if (result.has_value() && (flags & sc::OpenFlags::CreateOnly) == sc::OpenFlags::CreateOnly) {
        // File already exists, so we cannot create it.
        return EEXIST;
    } else if (result.has_error()) {
        return result.error();
    }

    VERIFY(result.has_value());

    if (stat_struct) {
        auto stat_region = EXPECTED_TRY(create_user_buffer(stat_struct, sizeof(sc::Stat), true));
        // Stat
        EXPECTED_TRY(stat_region.write_object(
            sc::Stat{.size = result.value()->size(),
                     .kind = result.value()->is_directory() ? sc::FileKind::Directory : sc::FileKind::File},
            0));
    }

    auto f = open_file(result.release_value());

    // TODO: Lock
    auto entity_fd = allocate_entity_handle_slot(f, 0);

    return entity_fd;
}
expected<long> Process::sys_read(int entity_handle, uSize offset, uPtr buffer, uSize len) {
    auto handle = EXPECTED_TRY(get_open_entity(entity_handle));

    if (!(handle->get_supported_operations() & EntityHandle::Read)) {
        return ENOTSUP;
    }

    // TODO: Verify ptr range
    auto mut_buffer = EXPECTED_TRY(create_user_buffer(buffer, len, false));
    auto x = handle->read(offset, mut_buffer).map_value([](auto x) { return static_cast<long>(x); });
    if (x.has_value()) DBG::dbgln("Read succeeded: {}"_sv, x.value());
    return x;
}
expected<long> Process::sys_write(int entity_handle, uSize offset, uPtr buffer, uSize len) {
    auto handle = EXPECTED_TRY(get_open_entity(entity_handle));

    if (!(handle->get_supported_operations() & EntityHandle::Write)) {
        return ENOTSUP;
    }

    // TODO: Verify ptr range
    auto mut_buffer = EXPECTED_TRY(create_user_buffer(buffer, len, true));
    return handle->write(offset, mut_buffer).map_value([](auto x) { return static_cast<long>(x); });
}
expected<long> Process::sys_close(int entity_handle) {
    // TODO: Lock
    // Cannot use get_entity_handle because we need direct access.
    if (entity_handle < 0) return EBADF;
    if (m_userspace_state->open_entities.size() <= static_cast<uSize>(entity_handle)) {
        return EBADF;
    }
    auto& entity_ptr = m_userspace_state->open_entities[entity_handle];
    if (!entity_ptr.handle) {
        return EBADF;
    } else {
        bek::shared_ptr<EntityHandle> temp{};
        bek::swap(entity_ptr.handle, temp);
    }

    return 0;
}
expected<long> Process::sys_get_directory_entries(int entity_handle, uSize offset, uPtr buffer, uSize len) {
    // FIXME: Very Inefficient
    auto handle = EXPECTED_TRY(get_open_entity(entity_handle));
    auto* file_handle = EntityHandle::as<fs::FileHandle>(*handle);
    if (!file_handle || !file_handle->entry().is_directory()) return ENOTDIR;

    UserBuffer user_buffer = EXPECTED_TRY(create_user_buffer(buffer, len, true));
    user_buffer.clear();
    uSize current_byte_offset = 0;

    auto entries = EXPECTED_TRY(file_handle->entry().all_children());

    // Will produce buffer of entries. The last entry will have next_offset == 0.

    while (offset < entries.size()) {
        auto& e = entries[offset];
        // Struct, plus string, plus null terminator.
        uSize entry_size = sc::FileListItem::whole_size(e->name().size());

        if ((current_byte_offset + entry_size) > user_buffer.size()) break;

        uSize offset_to_next = bek::align_up(entry_size, alignof(sc::FileListItem));

        if (offset + 1 == entries.size()) {
            // This is the final entry in the directory.
            offset_to_next = 0;
        } else if ((current_byte_offset + offset_to_next +
                    sc::FileListItem::whole_size(entries[offset + 1]->name().size())) > user_buffer.size()) {
            // The next entry will not fit in the buffer:
            // The offset should point beyond the buffer.
            offset_to_next = user_buffer.size() - current_byte_offset;
        }

        EXPECTED_TRY(user_buffer.write_object(
            sc::FileListItem{.next_offset = offset_to_next,
                             .size = e->size(),
                             .kind = (e->is_directory()) ? sc::FileKind::Directory : sc::FileKind::File,
                             ._name = {}},
            current_byte_offset));

        user_buffer.write_from(e->name().data(), e->name().size(),
                               current_byte_offset + sc::FileListItem::offset_of_name());
        // Null terminator provided by clear from before.

        current_byte_offset += offset_to_next;
        offset++;
    }

    return static_cast<long>(offset);
}
expected<long> Process::sys_seek(int entity_handle, sc::SeekLocation location, iSize offset) {
    auto handle = EXPECTED_TRY(get_open_entity(entity_handle));

    if (!(handle->get_supported_operations() & EntityHandle::Seek)) {
        return ENOTSUP;
    }

    return handle->seek(location, offset).map_value([](auto x) { return static_cast<long>(x); });
}
expected<long> Process::sys_stat(int entity_handle, uPtr path_str, uSize path_len, bool follow_symlinks,
                                 uPtr stat_struct) {
    // TODO: Symlinks
    (void)follow_symlinks;

    auto stat_region = EXPECTED_TRY(create_user_buffer(stat_struct, sizeof(sc::Stat), true));

    fs::EntryRef entry;

    if (entity_handle != sc::INVALID_ENTITY_ID) {
        auto handle = EXPECTED_TRY(get_open_entity(entity_handle));
        auto* file_handle = EntityHandle::as<fs::FileHandle>(*handle);
        if (!file_handle) return EBADF;
        entry = fs::EntryRef{&file_handle->entry()};
    } else {
        auto path_string = EXPECTED_TRY(read_string_from_user(path_str, path_len));
        auto the_path = EXPECTED_TRY(fs::path::parse_path(path_string));
        entry = EXPECTED_TRY(fs::fullPathLookup(m_userspace_state->cwd, the_path, nullptr));
    }

    EXPECTED_TRY(stat_region.write_object(
        sc::Stat{
            .size = entry->size(),
            .kind = entry->is_directory() ? sc::FileKind::Directory : sc::FileKind::File,
        },
        0));

    return (long)0;
}
expected<long> Process::sys_get_pid() { return m_pid; }
expected<long> Process::sys_allocate(uPtr address, uSize size, sc::AllocateFlags flags) {
    // This is ridiculously low?
    constexpr uSize maximum_allocation_size = 64 * MiB;
    if (size > maximum_allocation_size) {
        return ENOMEM;
    }

    if (size % PAGE_SIZE != 0) {
        return EINVAL;
    }

    bek::optional<uPtr> hint{};
    if (address != sc::INVALID_ADDRESS_VAL) {
        if ((address & PAGE_SIZE) != 0) return EINVAL;
        hint = address;
    }

    auto space = EXPECTED_TRY(mem::UserOwnedAllocation::create_contiguous(size / PAGE_SIZE));

    auto x = EXPECTED_TRY(m_userspace_state->address_space_manager.place_region(
        hint, MemoryOperation::Read | MemoryOperation::Write, bek::string{"Allocate"}, bek::move(space)));

    DBG::dbgln("Allocated space in {} ({}). Address space:"_sv, name(), pid());
    m_userspace_state->address_space_manager.debug_print();
    return static_cast<long>(x.start.get());
}
expected<long> Process::sys_deallocate(uPtr address, uSize size) {
    auto e = m_userspace_state->address_space_manager.deallocate_userspace_region(address, size);
    if (e == ESUCCESS) {
        return 0;
    } else {
        return e;
    }
}

expected<long> Process::sys_list_devices(uPtr buffer, uSize len, u64 protocol_filter) {
    bek::optional<DeviceProtocol> proto_filter =
        protocol_filter ? bek::optional{static_cast<DeviceProtocol>(protocol_filter)} : bek::nullopt;

    if (len == 0) {
        // Total up provisional size required.
        uSize total_bytes = 0;
        DeviceRegistry::the().for_each_device([&](bek::str_view name, bek::shared_ptr<Device>& dev) {
            if (!dev->userspace_protocol() || (proto_filter && proto_filter != dev->userspace_protocol())) {
                return IterationDecision::Continue;
            }
            auto entry_size = sc::DeviceListItem::whole_size(name.size());
            total_bytes = bek::align_up(total_bytes, alignof(sc::DeviceListItem)) + entry_size;
            return IterationDecision::Continue;
        });
        return static_cast<long>(total_bytes);
    }

    UserBuffer user_buffer = EXPECTED_TRY(create_user_buffer(buffer, len, true));
    user_buffer.clear();

    uSize current_byte_offset = 0;
    uSize offset_to_next = 0;
    ErrorCode error_status = ESUCCESS;

    DeviceRegistry::the().for_each_device([&](bek::str_view name, bek::shared_ptr<Device>& dev) {
        if (!dev->userspace_protocol() || (proto_filter && proto_filter != dev->userspace_protocol())) {
            return IterationDecision::Continue;
        }

        auto next_byte_offset = current_byte_offset + offset_to_next;
        auto entry_size = sc::DeviceListItem::whole_size(name.size());
        if ((next_byte_offset + entry_size) > user_buffer.size()) {
            error_status = EOVERFLOW;
            return IterationDecision::Break;
        }
        current_byte_offset = next_byte_offset;

        offset_to_next = bek::align_up(entry_size, alignof(sc::DeviceListItem));

        if (auto res = user_buffer.write_object(
                sc::DeviceListItem{.next_offset = offset_to_next, .protocol = *dev->userspace_protocol(), ._name = {}},
                current_byte_offset);
            res.has_error()) {
            error_status = res.error();
            return IterationDecision::Break;
        }

        if (auto res = user_buffer.write_from(name.data(), name.size(),
                                              current_byte_offset + sc::DeviceListItem::offset_of_name());
            res.has_error()) {
            error_status = res.error();
            return IterationDecision::Break;
        }

        return IterationDecision::Continue;
    });

    if (error_status == ESUCCESS) {
        // Reached end naturally.
        EXPECTED_TRY(user_buffer.write_object((u64)0, current_byte_offset));
        return 0l;
    } else if (error_status == EOVERFLOW) {
        // Ran out of space - set to pointer to end.
        EXPECTED_TRY(user_buffer.write_object((u64)user_buffer.size() - current_byte_offset, current_byte_offset));
        return error_status;
    } else {
        return error_status;
    }
}
expected<long> Process::sys_open_device(uPtr path_str, uPtr path_len) {
    auto path_string = EXPECTED_TRY(read_string_from_user(path_str, path_len));
    auto handle = DeviceRegistry::the().open(path_string.view());
    if (!handle) {
        return ENOENT;
    }

    // TODO: Lock
    auto entity_fd = allocate_entity_handle_slot(handle, 0);

    return entity_fd;
}
expected<long> Process::sys_message_device(int entity_handle, u64 id, uPtr buffer, uSize size) {
    UserBuffer user_buffer = EXPECTED_TRY(create_user_buffer(buffer, size, true));

    auto handle = EXPECTED_TRY(get_open_entity(entity_handle));

    if (!(handle->get_supported_operations() & EntityHandle::Message)) {
        return ENOTSUP;
    }

    return handle->message(id, user_buffer);
}

expected<long> Process::sys_fork(InterruptContext& ctx) {
    auto kernel_stack = mem::PageAllocator::the().allocate_region(m_kernel_stack.size / PAGE_SIZE);
    if (!kernel_stack) return ENOMEM;
    auto proc = bek::adopt_shared(new Process(m_name, this, *kernel_stack));
    if (!proc) {
        mem::PageAllocator::the().free_region(kernel_stack->start);
        return ENOMEM;
    }
    proc->m_userspace_state = UserspaceState{
        .user_stack_top = m_userspace_state->user_stack_top,
        .cwd = m_userspace_state->cwd,
        .address_space_manager = EXPECTED_TRY(m_userspace_state->address_space_manager.clone_for_fork()),
        .open_entities = m_userspace_state->open_entities,
    };

    auto current_user_stack = do_get_current_user_stack();

    proc->m_saved_registers =
        SavedRegisters::create_for_return_from_fork(ctx, proc->m_kernel_stack.end().ptr, current_user_stack);

    if (auto r = ProcessManager::the().register_process(proc); r != ESUCCESS) {
        return r;
    }
    m_children.push_back(proc.get());
    DBG::infoln("Forking process {}: forked address space:"_sv, proc->name());
    proc->m_userspace_state->address_space_manager.debug_print();
    proc->set_state(ProcessState::Running);
    return proc->pid();
}

expected<long> Process::sys_execute(uPtr executable_path, uSize path_len, uPtr args_array, uSize args_n, uPtr env_array,
                                    uSize env_n) {
    {
        using namespace fs;
        // Caution - path needs stable reference to path string.
        auto path_string = EXPECTED_TRY(read_string_from_user(executable_path, path_len));
        auto the_path = EXPECTED_TRY(path::parse_path(path_string));

        // TODO: Lock userspace state.
        EntryRef root = m_userspace_state->cwd;
        auto entry = EXPECTED_TRY(fullPathLookup(root, the_path, nullptr));

        bek::vector<bek::string> arguments;
        bek::vector<bek::string> environ;
        if (args_array && args_n) {
            bek::vector<bek::pair<uPtr, uSize>> arguments_ptrs(args_n);
            auto args_array_buffer = EXPECTED_TRY(create_user_buffer(args_array, args_n * 2 * sizeof(uPtr), false));
            EXPECTED_TRY(args_array_buffer.read_to(arguments_ptrs.data(), args_array_buffer.size(), 0));
            arguments.reserve(args_n);
            for (auto& arg_view : arguments_ptrs) {
                arguments.push_back(EXPECTED_TRY(read_string_from_user(arg_view.first, arg_view.second)));
            }
        }

        if (env_array && env_n) {
            bek::vector<bek::pair<uPtr, uSize>> env_ptrs(env_n);
            auto env_array_buffer = EXPECTED_TRY(create_user_buffer(env_array, env_n * 2 * sizeof(uPtr), false));
            EXPECTED_TRY(env_array_buffer.read_to(env_ptrs.data(), env_array_buffer.size(), 0));
            environ.reserve(args_n);
            for (auto& env_view : env_ptrs) {
                environ.push_back(EXPECTED_TRY(read_string_from_user(env_view.first, env_view.second)));
            }
        }

        auto exec_result =
            execute_executable(entry, m_userspace_state->cwd, bek::move(m_userspace_state->open_entities),
                               bek::move(arguments), bek::move(environ));

        // execute_executable enters critical if altering current process's saved regs.
        VERIFY(ProcessManager::the().is_critical());
        if (exec_result != ESUCCESS) {
            ProcessManager::the().exit_critical();
            return exec_result;
        }
    }
    // At this point,
    // (a) we are in critical section (for our own processor!)
    // (b) SavedRegisters has been set and is volatile.
    // (c) The current kernel stack *must* contain no RAII elements, or resources to be freed.

    do_switch_user_address_space(m_userspace_state->address_space_manager.raw_root_ptr());

    // (i) Disable interrupts
    disable_interrupts();
    // (ii) leave critical
    ProcessManager::the().exit_critical();
    VERIFY(!ProcessManager::the().is_critical());

    do_assume_process_state(m_saved_registers, 0);
    ASSERT_UNREACHABLE();
}
expected<long> Process::sys_create_pipe(uPtr pipe_handles_struct, u64 raw_flags) {
    auto flags = sc::CreatePipeHandleFlags::from(raw_flags);
    auto pipe = bek::adopt_shared(new Pipe());

    auto pipe_handles_buffer =
        EXPECTED_TRY(create_user_buffer(pipe_handles_struct, sizeof(sc::CreatePipeHandles), true));

    auto read_handle = bek::adopt_shared(new PipeHandle(pipe, true, flags.read_blocking));
    auto write_handle = bek::adopt_shared(new PipeHandle(pipe, false, flags.write_blocking));

    sc::CreatePipeHandles handles = {allocate_entity_handle_slot(read_handle, flags.read_group),
                                     allocate_entity_handle_slot(write_handle, flags.write_group)};

    EXPECTED_TRY(pipe_handles_buffer.write_object(handles));
    return 0l;
}

expected<long> Process::sys_duplicate(long handle_slot, long new_handle_slot, u8 group) {
    auto handle = EXPECTED_TRY(get_open_entity(handle_slot));

    if (new_handle_slot == sc::INVALID_ENTITY_ID) {
        return allocate_entity_handle_slot(handle, group);
    } else {
        if (new_handle_slot < 0) return EINVAL;
        // FIXME: Extend
        if (new_handle_slot >= m_userspace_state->open_entities.size()) return EINVAL;

        auto& old_handle = m_userspace_state->open_entities[new_handle_slot];
        old_handle.group = group;
        old_handle.handle = bek::move(handle);
        return new_handle_slot;
    }
}
expected<long> Process::sys_wait(long pid, uPtr status_ptr, u64 flags) {
    bek::optional<UserBuffer> status_buffer =
        status_ptr ? bek::optional{EXPECTED_TRY(create_user_buffer(status_ptr, sizeof(int), true))} : bek::nullopt;
    if (pid > 0) {
        for (auto child : m_children) {
            if (child->pid() == pid) {
                while (child->m_running_state != ProcessState::AwaitingDeath) {
                    // Bloop
                }

                if (status_buffer) {
                    auto exit_code = child->m_exit_code ? *child->m_exit_code : -1;
                    status_buffer->write_from(&exit_code, sizeof(exit_code), 0);
                }
                for (auto& current_child : m_children) {
                    if (current_child->pid() == pid) {
                        m_children.extract(current_child);
                    }
                }
                ProcessManager::the().reap_process(*child);
                return pid;
            }
        }
        return ECHILD;
    } else if (pid == -1) {
        while (m_children.size() > 0) {
            for (auto& child : m_children) {
                if (child->m_running_state == ProcessState::AwaitingDeath) {
                    if (status_buffer) {
                        auto exit_code = child->m_exit_code ? *child->m_exit_code : -1;
                        status_buffer->write_from(&exit_code, sizeof(exit_code), 0);
                    }
                    auto child_pid = child->pid();
                    m_children.extract(child);
                    ProcessManager::the().reap_process(*child);
                    return child_pid;
                }
            }
        }
        return ECHILD;
    } else {
        return EINVAL;
    }
}
expected<long> Process::sys_chdir(uPtr path_str, uSize path_len) {
    auto path_string = EXPECTED_TRY(read_string_from_user(path_str, path_len));
    auto the_path = EXPECTED_TRY(fs::path::parse_path(path_string));

    // TODO: Lock userspace state.
    fs::EntryRef root = m_userspace_state->cwd;
    m_userspace_state->cwd = EXPECTED_TRY(fullPathLookup(root, the_path, nullptr));
    return ESUCCESS;
}
