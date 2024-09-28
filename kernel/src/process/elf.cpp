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

#include "process/elf.h"

#include "library/debug.h"
#include "mm/memory_manager.h"

using DBG = DebugScope<"Elf", DebugLevel::WARN>;

struct elf_file_header {
    enum word_width_t : u8 { W32BIT = 1, W64BIT = 2 };
    enum endianness_t : u8 { LITTLE_ENDIAN = 1, BIG_ENDIAN = 2 };
    enum type_t : u16 {
        ET_NONE = 0,
        ET_REL = 1,
        ET_EXEC = 2,
        ET_DYN = 3,
        ET_CORE = 4,
        ET_LOOS = 0xfe00,
        ET_HIOS = 0xfeff,
        ET_LOPROC = 0xff00,
        ET_HIPROC = 0xffff
    };
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

constexpr inline u8 ELF_MAGIC[4] = {0x7F, 'E', 'L', 'F'};

struct elf_section_header {
    u32 name_offset;
    enum type_t : u32 {
        SHT_NULL = 0,
        SHT_PROGBITS = 1,
        SHT_SYMTAB = 2,
        SHT_STRTAB = 3,
        SHT_RELA = 4,
        SHT_HASH = 5,
        SHT_DYNAMIC = 6,
        SHT_NOTE = 7,
        SHT_NOBITS = 8,
        SHT_REL = 9,
        SHT_SHLIB = 0xA,
        SHT_DYNSYM = 0xB,
        SHT_INIT_ARRAY = 0xE,
        SHT_FINI_ARRAY = 0xF,
        SHT_PREINIT_ARRAY = 0x10,
        SHT_GROUP = 0x11,
        SHT_SYMTAB_SHNDX = 0x12,
        SHT_NUM = 0x13
    };
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

constexpr inline u32 ELF_PROG_EXEC = 1;
constexpr inline u32 ELF_PROG_WRITE = 2;
constexpr inline u32 ELF_PROG_READ = 4;

bek::string create_region_name(bek::str_view fname, MemoryOperation operations) {
    auto r = (!!(operations & MemoryOperation::Read)) ? "r" : "-";
    auto w = (!!(operations & MemoryOperation::Write)) ? "w" : "-";
    auto x = (!!(operations & MemoryOperation::Execute)) ? "x" : "-";
    return bek::format("[{}]({}{}{})"_sv, fname, r, w, x);
}

expected<bek::own_ptr<ElfFile>> ElfFile::parse_file(fs::EntryRef file) {
    BitwiseObjectBuffer<elf_file_header> file_header_buffer{{}};
    auto& header = file_header_buffer.object();
    if (EXPECTED_TRY(file->read_bytes(file_header_buffer, 0, file_header_buffer.size())) < file_header_buffer.size()) {
        return ENOEXEC;
    }
    if (bek::mem_compare(header.magic_number, ELF_MAGIC, 4)) {
        // Not a valid ELF file.
        DBG::warnln("ELF Header magic number invalid."_sv);
        return ENOEXEC;
    }

    if (header.endianness != elf_file_header::LITTLE_ENDIAN || header.word_width != elf_file_header::W64BIT) {
        DBG::warnln("ELF is not 64-bit, little endian."_sv);
        return ENOTSUP;
    }

    if (header.machine != ELF_MACHINE_AARCH64) {
        DBG::warnln("ELF file architecture is {}, not A64"_sv, header.machine);
        return ENOTSUP;
    }

    if (header.obj_type != elf_file_header::ET_EXEC) {
        DBG::warnln("ELF file object type is {}, not ET_EXEC."_sv, (uSize)header.obj_type);
        return ENOEXEC;
    }

    // TODO: Check a whole bunch of stuff

    DBG::infoln("ELF file has {} headers."_sv, header.program_header_entry_count);
    if (header.program_header_entry_size != sizeof(elf_program_header)) {
        DBG::errln("ELF file program headers are size {}. Should be {}."_sv, header.program_header_entry_size,
                   sizeof(elf_program_header));
        return ENOTSUP;
    }

    bek::vector<elf_program_header> headers{header.program_header_entry_count};
    auto headers_buf = KernelBuffer(headers.data(), headers.size() * sizeof(elf_program_header));
    if (EXPECTED_TRY(file->read_bytes(headers_buf, header.program_header_offset, headers_buf.size())) <
        headers_buf.size()) {
        return ENOEXEC;
    }
    bek::optional<mem::UserRegion> maximal_region;
    for (auto& prog_hdr : headers) {
        if (prog_hdr.type == elf_program_header::PT_LOAD) {
            if (prog_hdr.virtual_address >= USER_ADDR_MAX || prog_hdr.memory_size >= USER_ADDR_MAX ||
                (prog_hdr.virtual_address + prog_hdr.memory_size) >= USER_ADDR_MAX) {
                return ENOTSUP;
            }
            auto region = mem::UserRegion{prog_hdr.virtual_address, prog_hdr.memory_size};
            if (!maximal_region) {
                maximal_region = region;
            } else {
                if (maximal_region->start > region.start) maximal_region->start = region.start;
                if (maximal_region->end() < region.end()) maximal_region->size = region.end() - maximal_region->start;
            }

            // Ensure doesn't go beyond bounds.
            if (prog_hdr.offset + prog_hdr.file_size > file->size()) {
                DBG::warnln("Error: ELF file program header refers to data beyond ELF file."_sv);
                return ENOEXEC;
            }

            if (prog_hdr.memory_size < prog_hdr.file_size) {
                DBG::warnln("Error: ELF file program header too small in memory for data."_sv);
                return ENOEXEC;
            }
        } else if (prog_hdr.type == elf_program_header::PT_INTERP) {
            DBG::warnln("Error: Executable is meant to be loaded with a dynamic linker."_sv);
            return ENOTSUP;
        }
    }
    if (!maximal_region) {
        DBG::warnln("Error: Executable has no loadable program sections."_sv);
        return ENOEXEC;
    }

    if (header.entry_point >= USER_ADDR_MAX) {
        DBG::warnln("Invalid entry point: {}"_sv, header.entry_point);
        return ENOTSUP;
    }
    mem::UserPtr entry_point{header.entry_point};

    return bek::own_ptr{new ElfFile(bek::move(file), bek::move(headers), entry_point, *maximal_region)};
}

ElfFile::ElfFile(fs::EntryRef file, bek::vector<elf_program_header> program_headers, mem::UserPtr entry_point,
                 mem::UserRegion program_range)
    : m_file(bek::move(file)),
      m_program_headers(bek::move(program_headers)),
      m_entry_point(entry_point),
      m_program_range(program_range) {}

expected<uPtr> ElfFile::load_into(SpaceManager& space) {
    for (auto& hdr : m_program_headers) {
        if (hdr.type == elf_program_header::PT_LOAD) {
            // At this point, we've checked a bunch of things already.
            // We can confidently go ahead and load!...
            if (hdr.memory_size == 0) continue;

            mem::UserRegion target_region{hdr.virtual_address, hdr.memory_size};
            mem::UserRegion aligned_region = target_region.align_to_page();
            auto region_start_offset = target_region.start - aligned_region.start;
            VERIFY(region_start_offset >= 0);

            MemoryOperation operations =
                ((hdr.flags & ELF_PROG_READ) ? MemoryOperation::Read : MemoryOperation::None) |
                ((hdr.flags & ELF_PROG_WRITE) ? MemoryOperation::Write : MemoryOperation::None) |
                ((hdr.flags & ELF_PROG_EXEC) ? MemoryOperation::Execute : MemoryOperation::None);

            if ((operations & (MemoryOperation::Write | MemoryOperation::Execute)) ==
                (MemoryOperation::Write | MemoryOperation::Execute)) {
                DBG::warnln("Warn: *Severe Danger* region is mapped both writeable and executable."_sv);
            }

            bek::string name = create_region_name(m_file->name(), operations);
            auto region = EXPECTED_TRY(space.allocate_placed_region(aligned_region, operations, name.view()));
            auto kernel_memory_region = region->kernel_mapped_region();

            // Now we populate
            if (region_start_offset) {
                bek::memset(kernel_memory_region.start.get(), 0, region_start_offset);
            }

            KernelBuffer data_buffer{kernel_memory_region.start.offset(region_start_offset).get(), hdr.file_size};
            auto length = EXPECTED_TRY(m_file->read_bytes(data_buffer, hdr.offset, hdr.file_size));
            if (length < hdr.file_size) return EIO;

            if (region_start_offset + hdr.file_size < kernel_memory_region.size) {
                bek::memset(kernel_memory_region.start.offset(region_start_offset + hdr.file_size).get(), 0,
                            kernel_memory_region.size - (region_start_offset + hdr.file_size));
            }
        }
    }

    // We've loaded all the headers!
    return 0ull;
}
mem::UserRegion ElfFile::get_sensible_stack_region(uSize maximum_size) const {
    constexpr uSize desirable_null_buffer = 16 * PAGE_SIZE;
    constexpr uSize desirable_region_space = 2 * PAGE_SIZE;
    auto aligned_program_range = m_program_range.align_to_page();

    if (auto top = mem::UserPtr{desirable_null_buffer + maximum_size};
        top.offset(desirable_region_space) <= aligned_program_range.start) {
        return {top - maximum_size, maximum_size};
    } else {
        return {aligned_program_range.end().offset(desirable_region_space), maximum_size};
    }
}
mem::UserPtr ElfFile::get_entry_point() const { return m_entry_point; }
