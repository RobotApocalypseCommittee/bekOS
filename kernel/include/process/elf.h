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
#ifndef BEKOS_ELF_H
#define BEKOS_ELF_H

#include <filesystem/filesystem.h>

#include "process/process.h"

// We only bother with elf-64, because sixty-four > thirty-two

struct elf_program_header {
    enum type_t : u32 {
        PT_NULL = 0,
        PT_LOAD = 1,
        PT_DYNAMIC = 2,
        PT_INTERP = 3,
        PT_NOTE = 4,
        PT_SHLIB = 5,
        PT_PHDR = 6,
        PT_TLS = 7,
        PT_LOOS = 0x60000000,
        PT_HIOS = 0x6FFFFFFF,
        PT_LOPROC = 0x70000000,
        PT_HIPROC = 0x7FFFFFFF
    };
    type_t type;
    u32 flags;
    u64 offset;
    u64 virtual_address;
    u64 physical_address;
    u64 file_size;
    u64 memory_size;
    u64 align;
};

static_assert(sizeof(elf_program_header) == 56);

class ElfFile {
public:
    static expected<bek::own_ptr<ElfFile>> parse_file(fs::EntryRef file);
    expected<uPtr> load_into(SpaceManager& space);

    mem::UserRegion get_sensible_stack_region(uSize maximum_size) const;
    mem::UserPtr get_entry_point() const;

private:
    ElfFile(fs::EntryRef file, bek::vector<elf_program_header> program_headers, mem::UserPtr entry_point,
            mem::UserRegion program_range);
    fs::EntryRef m_file;
    bek::vector<elf_program_header> m_program_headers;
    mem::UserPtr m_entry_point;
    mem::UserRegion m_program_range;
};

#endif  // BEKOS_ELF_H
