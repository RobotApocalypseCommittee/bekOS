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
#ifndef BEKOS_ELF_H
#define BEKOS_ELF_H

#include <hardtypes.h>
#include <filesystem/filesystem.h>
#include <page_mapping.h>
#include "process/process.h"

// We only bother with elf-64, because sixty-four > thirty-two


struct elf_file_header {
    enum word_width_t: u8 { W32BIT = 1, W64BIT = 2};
    enum endianness_t: u8 { LITTLE_ENDIAN = 1, BIG_ENDIAN = 2};
    enum type_t: u16 { ET_NONE = 0, ET_REL = 1, ET_EXEC = 2, ET_DYN = 3, ET_CORE = 4,
            ET_LOOS = 0xfe00, ET_HIOS = 0xfeff, ET_LOPROC = 0xff00, ET_HIPROC = 0xffff};
    u8 magic_number[4];
    word_width_t word_width;
    endianness_t endianness;
    u8 version_ident;
    u8 os_abi;
    u8 abi_version;
    u8 _unused[7];
    type_t obj_type;
#define ELF_MACHINE_AARCH64 (0xB7)
    u16 machine;
    u32 version;
    u64 entry_point;
    u64 program_header_offset;
    u64 section_header_offset;
    u32 flags;
    u16 file_header_size;
    u16 program_header_entry_size;
    u16 program_header_entry_count;
    u16 section_header_entry_size;
    u16 section_header_entry_count;
    u16 section_names_index;
} __attribute__((packed));

struct elf_program_header {
    enum type_t: u32 {PT_NULL = 0, PT_LOAD = 1, PT_DYNAMIC = 2, PT_INTERP = 3,
            PT_NOTE = 4, PT_SHLIB = 5, PT_PHDR = 6, PT_TLS = 7,
            PT_LOOS = 0x60000000, PT_HIOS = 0x6FFFFFFF, PT_LOPROC = 0x70000000, PT_HIPROC = 0x7FFFFFFF};
    type_t type;
    u32 flags;
    u64 offset;
    u64 virtual_address;
    u64 physical_address;
    u64 file_size;
    u64 memory_size;
    u64 align;
} __attribute__((packed));

struct elf_section_header {
    u32 name_offset;
    enum type_t: u32 {SHT_NULL = 0, SHT_PROGBITS = 1, SHT_SYMTAB = 2, SHT_STRTAB = 3, SHT_RELA = 4, SHT_HASH = 5, SHT_DYNAMIC = 6, SHT_NOTE = 7, SHT_NOBITS = 8, SHT_REL = 9, SHT_SHLIB = 0xA, SHT_DYNSYM = 0xB, SHT_INIT_ARRAY = 0xE, SHT_FINI_ARRAY = 0xF, SHT_PREINIT_ARRAY = 0x10, SHT_GROUP = 0x11, SHT_SYMTAB_SHNDX = 0x12, SHT_NUM = 0x13};
    type_t type;
    u64 flags;
    u64 virtual_address;
    u64 file_offset;
    u64 file_size;
    u32 link;
    u32 info;
    u64 address_align;
    u64 entry_size;
} __attribute__((packed));

class elf_file {
public:
    explicit elf_file(File* f);

    bool parse();

    bool load(Process* process);

    u64 getEntryPoint();
    u64 decideStackAddress();


private:
    File* f;
    bek::vector<elf_program_header> m_important_headers;
    u64 entry_point;
    u64 lowest_segment = -1;

};

void elf_loader_fn(u64 elf_object);

#endif //BEKOS_ELF_H
