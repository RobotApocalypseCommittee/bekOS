// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_SYSCALLS_H
#define BEKOS_SYSCALLS_H

#include "bek/bitwise_enum.h"
#include "bek/memory.h"
#include "bek/str.h"
#include "bek/types.h"
#include "bek/utility.h"
#include "device_protocols.h"

namespace sc {

enum class SysCall : u64 {
    // File Operations
    Open,
    Close,
    Read,
    Write,
    Seek,
    Stat,
    GetDirEntries,
    Duplicate,
    // Device Operations
    ListDevices,
    OpenDevice,
    CommandDevice,
    // Memory Operations
    Allocate,
    Deallocate,
    // IPC
    CreatePipe,
    // Process
    GetPid,
    Fork,
    Exec,
    Exit,
    Wait,
    ChangeWorkingDirectory,
    // Internal Socket
    InterlinkAdvertise,
    InterlinkConnect,
    InterlinkAccept,
    InterlinkSend,
    InterlinkReceive,
    // Miscellaneous
    Sleep,
    GetTicks,
};

enum class OpenFlags {
    None = 0x00,
    Read = 0x01,
    Write = 0x02,
    CreateIfMissing = 0x04,
    CreateOnly = 0x08,
    DIRECTORY = 0x10,
};

// BEK_ENUM_BITWISE_OPS(OpenFlags)

enum class FileKind : u8 {
    File,
    Directory,
};

struct Stat {
    u64 size;
    FileKind kind;
};

enum class SeekLocation {
    Start,
    Current,
    End,
};

inline constexpr int INVALID_ENTITY_ID = __INT32_MAX__;
inline constexpr uSize INVALID_OFFSET_VAL = (-1ull);
inline constexpr uSize INVALID_ADDRESS_VAL = (-1ul);

struct CreatePipeHandles {
    long read_handle;
    long write_handle;
};

struct CreatePipeHandleFlags {
    u8 read_group;
    u8 write_group;
    bool read_blocking;
    bool write_blocking;
    operator u64() const { return bek::bit_cast<u32>(*this); }  // NOLINT(*-explicit-constructor)
    static CreatePipeHandleFlags from(u64 v) { return bek::bit_cast<CreatePipeHandleFlags>(static_cast<u32>(v)); }
};

enum class AllocateFlags { None = 0 };

struct DeviceListItem {
    /// Offset from this structure to next Item. If 0, this means EOF. If = to end or beyond buffer, means get next
    /// buffer.
    u64 next_offset;
    DeviceProtocol protocol;
    // Length of four is to optimise packing - in reality unlimited.
    char _name[4];

    const char* name() const { return &_name[0]; }

    char* name() { return &_name[0]; }

    constexpr static uSize offset_of_name() { return OFFSETOF(DeviceListItem, _name); }

    /// Returns size taken up by struct
    /// \param name_len Length of name, *excluding* null terminator.
    /// \return Bytes taken up by struct, not including padding.
    constexpr static uSize whole_size(uSize name_len) { return sizeof(DeviceListItem) - sizeof(_name) + name_len + 1; }

    bek::str_view name_view() const {
        auto len = bek::strlen(_name, next_offset - offset_of_name());
        return {_name, len};
    }
};

static_assert(sizeof(DeviceListItem) == 16);
static_assert(alignof(DeviceListItem) == 8);

struct FileListItem {
    u64 next_offset;
    u64 size;
    FileKind kind;
    // Length of seven is to optimise packing - in reality unlimited.
    char _name[7];

    const char* name() const { return &_name[0]; }

    char* name() { return &_name[0]; }

    constexpr static uSize offset_of_name() { return OFFSETOF(FileListItem, _name); }

    /// Returns size taken up by struct
    /// \param name_len Length of name, *excluding* null terminator.
    /// \return Bytes taken up by struct, not including padding.
    constexpr static uSize whole_size(uSize name_len) { return sizeof(FileListItem) - sizeof(_name) + name_len + 1; }

    bek::str_view name_view() const {
        auto len = bek::strlen(_name, next_offset ? next_offset - offset_of_name() : 1024);
        return {_name, len};
    }
};

static_assert(sizeof(FileListItem) == 24);
static_assert(alignof(FileListItem) == 8);

}  // namespace sc

template <>
constexpr inline bool bek_is_bitwise_enum<sc::OpenFlags> = true;

#endif  // BEKOS_SYSCALLS_H
