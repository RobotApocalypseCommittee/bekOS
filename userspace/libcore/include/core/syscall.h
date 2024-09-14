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

#ifndef BEKOS_CORE_SYSCALL_H
#define BEKOS_CORE_SYSCALL_H

#include "api/syscalls.h"
#include "bek/span.h"
#include "bek/str.h"
#include "bek/types.h"
#include "error.h"

namespace core::syscall {

extern "C" {
u64 syscall0(u64 sc);
u64 syscall1(u64 sc, u64 a1);
u64 syscall2(u64 sc, u64 a1, u64 a2);
u64 syscall3(u64 sc, u64 a1, u64 a2, u64 a3);
u64 syscall4(u64 sc, u64 a1, u64 a2, u64 a3, u64 a4);
u64 syscall5(u64 sc, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5);
u64 syscall6(u64 sc, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);
}

ALWAYS_INLINE u64 syscall(sc::SysCall sc) { return syscall0(static_cast<u64>(sc)); }
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1) { return syscall1(static_cast<u64>(sc), a1); }
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1, u64 a2) { return syscall2(static_cast<u64>(sc), a1, a2); }
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1, u64 a2, u64 a3) { return syscall3(static_cast<u64>(sc), a1, a2, a3); }
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1, u64 a2, u64 a3, u64 a4) {
    return syscall4(static_cast<u64>(sc), a1, a2, a3, a4);
}
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5) {
    return syscall5(static_cast<u64>(sc), a1, a2, a3, a4, a5);
}
ALWAYS_INLINE u64 syscall(sc::SysCall sc, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    return syscall6(static_cast<u64>(sc), a1, a2, a3, a4, a5, a6);
}

/// Syscall: Opens a file at path and provides entity descriptor.
/// \param path The file path
/// \param flags Flags which dictate mode in which file is when opened.
/// \param parent Either file descriptor of parent (from which relative path should be taken),
/// otherwise must be -1.
/// \param stat_struct Either nullptr, or pointer to stat struct which will be populated if opening
/// is successful.
/// \return Error, or file descriptor (positive)
expected<long> open(bek::str_view path, sc::OpenFlags flags, int parent, void* stat_struct);

/// Syscall: Reads at least `length` bytes from an open file or device.
/// \param entity_handle Handle on open entity.
/// \param offset Offset from which to read. Must be 0 if entity non-seekable. Shall be -1 to read
/// from entity's current seek location. Other negative values disallowed.
/// \param buffer Buffer of contiguous bytes to which to read. Must be at least `length` long.
/// \param length Maximum number of bytes to read.
/// \return Error, or number of bytes read (positive).
expected<long> read(int entity_handle, uSize offset, void* buffer, uSize length);

/// Syscall: Writes at least `length` bytes to an open file or device.
/// \param entity_handle Handle on open entity.
/// \param offset Offset at which to write. Must be 0 if entity non-seekable. Shall be -1 to read
/// from entity's current seek location. Other negative values disallowed.
/// \param buffer Buffer of contiguous bytes from which to write. Must be at least `length` long.
/// \param length Maximum number of bytes to write.
/// \return Error, or number of bytes written (positive).
expected<long> write(int entity_handle, uSize offset, const void* buffer, uSize length);

expected<long> message(long entity_handle, u64 id, void* buffer, uSize length);

/// Syscall: Sets kernel-side location in entity for open entity, and returns the current cursor
/// offset from beginning. Seekable entities only.
/// \param entity_handle Handle on seekable open entity.
/// \param location Relative to which position to seek.
/// \param offset Offset from `location` (may be negative provided location + offset not outside
/// entity).
/// \return Error, or offset from beginning of cursor (positive).
expected<long> seek(int entity_handle, sc::SeekLocation location, iSize offset);

/// Closes an open entity, leaving handle free to be re-used.
/// \param entity_handle Handle to open entity.
/// \return
expected<long> close(int entity_handle);

expected<long> get_directory_entries(int entity_handle, uSize offset, void* buffer, uSize len);
expected<uPtr> allocate(uPtr address_hint, uSize size, sc::AllocateFlags flags);
expected<uPtr> deallocate(uPtr address, uSize size);

expected<long> open_device(bek::str_view path);

ErrorCode list_devices(void* buffer, uSize len);
ErrorCode list_devices(void* buffer, uSize len, DeviceProtocol protocol_filter);

expected<int> get_pid();

[[noreturn]] void exit(int code);

expected<long> fork();

core::expected<long> exec(bek::str_view path, bek::span<bek::str_view> arguments, bek::span<bek::str_view> environ);

expected<sc::CreatePipeHandles> create_pipe(sc::CreatePipeHandleFlags flags);

expected<long> duplicate(long old_slot, long new_slot, u8 group);

void sleep(uSize microseconds);

expected<long> wait(long pid, int& status);
}  // namespace core::syscall

#endif  // BEKOS_CORE_SYSCALL_H
