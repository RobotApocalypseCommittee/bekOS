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

#include "core/syscall.h"

template <typename T>
u64 to_syscall_arg(T v) {
    return static_cast<u64>(v);
}

template <typename T>
u64 to_syscall_arg(T* v) {
    return reinterpret_cast<uPtr>(v);
}

template <typename T>
u64 to_syscall_arg(const T* v) {
    return reinterpret_cast<uPtr>(v);
}

template <typename Ret, typename... Args>
core::expected<Ret> syscall_to_result(sc::SysCall sc, Args... args) {
    i64 val = core::syscall::syscall(sc, to_syscall_arg(args)...);
    if (val < 0) {
        return static_cast<ErrorCode>(-val);
    } else {
        return static_cast<Ret>(val);
    }
}

template <typename... Args>
ErrorCode syscall_to_error_code(sc::SysCall sc, Args... args) {
    i64 val = core::syscall::syscall(sc, to_syscall_arg(args)...);

    if (val <= 0) {
        return static_cast<ErrorCode>(-val);
    } else {
        // TODO: Do something
        return EFAIL;
    }
}

core::expected<long> core::syscall::open(bek::str_view path, sc::OpenFlags flags, int parent, void* stat_struct) {
    return syscall_to_result<long>(sc::SysCall::Open, path.data(), path.size(), flags, parent, stat_struct);
}
core::expected<long> core::syscall::read(int entity_handle, uSize offset, void* buffer, uSize length) {
    return syscall_to_result<long>(sc::SysCall::Read, entity_handle, offset, buffer, length);
}
core::expected<long> core::syscall::write(int entity_handle, uSize offset, const void* buffer, uSize length) {
    return syscall_to_result<long>(sc::SysCall::Write, entity_handle, offset, buffer, length);
}
core::expected<long> core::syscall::seek(int entity_handle, sc::SeekLocation location, iSize offset) {
    return syscall_to_result<long>(sc::SysCall::Seek, entity_handle, location, offset);
}
core::expected<long> core::syscall::close(int entity_handle) {
    return syscall_to_result<long>(sc::SysCall::Close, entity_handle);
}
core::expected<long> core::syscall::get_directory_entries(int entity_handle, uSize offset, void* buffer, uSize len) {
    return syscall_to_result<long>(sc::SysCall::GetDirEntries, entity_handle, offset, buffer, len);
}
core::expected<uPtr> core::syscall::allocate(uPtr address, uSize size, sc::AllocateFlags flags) {
    return syscall_to_result<uPtr>(sc::SysCall::Allocate, address, size, flags);
}
core::expected<uPtr> core::syscall::deallocate(uPtr address, uSize size) {
    return syscall_to_result<uPtr>(sc::SysCall::Deallocate, address, size);
}
core::expected<int> core::syscall::get_pid() { return syscall_to_result<int>(sc::SysCall::GetPid); }
core::expected<long> core::syscall::open_device(bek::str_view path) {
    return syscall_to_result<long>(sc::SysCall::OpenDevice, path.data(), path.size());
}
ErrorCode core::syscall::list_devices(void* buffer, uSize len) {
    return syscall_to_error_code(sc::SysCall::ListDevices, buffer, len, 0);
}
ErrorCode core::syscall::list_devices(void* buffer, uSize len, DeviceProtocol protocol_filter) {
    return syscall_to_error_code(sc::SysCall::ListDevices, buffer, len, protocol_filter);
}
void core::syscall::exit(int code) {
    syscall(sc::SysCall::Exit, code);
    ASSERT_UNREACHABLE();
}
core::expected<long> core::syscall::message(long entity_handle, u64 id, void* buffer, uSize length) {
    return syscall_to_result<long>(sc::SysCall::CommandDevice, entity_handle, id, buffer, length);
}
core::expected<long> core::syscall::fork() { return syscall_to_result<long>(sc::SysCall::Fork); }
void core::syscall::wait(uSize microseconds) { syscall(sc::SysCall::Sleep, microseconds); }
