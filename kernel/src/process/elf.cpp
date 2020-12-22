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

#include <utils.h>
#include <process/process_entry.h>
#include <mm.h>
#include <printf.h>
#include "process/elf.h"

extern memory_manager memoryManager;

elf_file::elf_file(fs::File *f) : f(f) {}

bool elf_file::parse() {
    elf_file_header fileHeader;
    f->read(&fileHeader, sizeof(fileHeader), 0);
    if (fileHeader.magic_number[0] != 0x7F || strncmp((char*)&fileHeader.magic_number[1], "ELF", 3) != 0) {
        // Is invalid
        return false;
    }
    if (fileHeader.endianness != elf_file_header::LITTLE_ENDIAN || fileHeader.word_width != elf_file_header::W64BIT) {
        // Only (currently) supports 64-bit, little-endian executables
        return false;
    }
    // TODO: Check a whole bunch of stuff
    // Now we load the Program Headers:
    auto programHeaders = new elf_program_header[fileHeader.program_header_entry_count];
    f->read(programHeaders, sizeof(elf_program_header)* fileHeader.program_header_entry_count, fileHeader.program_header_offset);
    for (int i = 0; i < fileHeader.program_header_entry_count; i++) {
        // Loop through program headers and store useful ones
        if (programHeaders[i].type == elf_program_header::PT_LOAD) {
            m_important_headers.push_back(programHeaders[i]);
            if (lowest_segment > programHeaders[i].virtual_address) {
                lowest_segment = programHeaders[i].virtual_address;
            }
        } else {
            // TODO: Handle others
            printf("Unknown segment type %d", programHeaders[i].type);
        }
    }
    entry_point = fileHeader.entry_point;
    return true;
}

bool elf_file::load(Process* process) {
    // Loop through and correctly allocate
    for (uSize i = 0; i < m_important_headers.size(); i++) {
        u64 memorySize = m_important_headers[i].memory_size;
        u64 fileSize = m_important_headers[i].file_size;

        // The section may start after the start of a page
        u64 virtual_page_start = (m_important_headers[i].virtual_address / PAGE_SIZE) * PAGE_SIZE;
        u64 virtual_offset = m_important_headers[i].virtual_address - virtual_page_start;
        u64 page_no = ((memorySize + virtual_offset) + PAGE_SIZE - 1) / 4096;

        uPtr region_phys = memoryManager.reserve_region(page_no, PAGE_KERNEL);
        for (uSize pn = 0; pn < page_no; pn++) {
            process->userPages.push_back(region_phys + pn*PAGE_SIZE);
            process->translationTable.map(virtual_page_start + pn * PAGE_SIZE, region_phys + pn * PAGE_SIZE);
        }
        // Copy the data
        f->read(reinterpret_cast<void*>(phys_to_virt(region_phys) + virtual_offset), fileSize, m_important_headers[i].offset);
        if (fileSize < memorySize) {
            memset(reinterpret_cast<void*>(phys_to_virt(region_phys) + virtual_offset + fileSize), 0, memorySize -
                                                                                                      fileSize);
        }
    }
    return true;
}

u64 elf_file::getEntryPoint() {
    assert(entry_point != 0);
    return entry_point;
}

u64 elf_file::decideStackAddress() {
    return (lowest_segment - 1) & (-PAGE_SIZE);
}

void elf_loader_fn(u64 elf_object) {
    auto* elf = reinterpret_cast<elf_file*>(elf_object);
    processManager->disablePreempt();
    Process* p = processManager->getCurrentProcess();
    elf->load(p);
    // Allocate some memory for the stack
    uPtr stackpage = memoryManager.reserve_region(1, PAGE_KERNEL);
    p->userPages.push_back(stackpage);
    p->translationTable.map(elf->decideStackAddress() - PAGE_SIZE, stackpage);
    p->user_stack_top = elf->decideStackAddress();
    set_usertable(p->translationTable.get_table_address());
    processManager->enablePreempt();
    become_userspace(elf->getEntryPoint(), elf->decideStackAddress());
}
