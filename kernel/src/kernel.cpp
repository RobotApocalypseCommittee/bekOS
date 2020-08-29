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

#include <stddef.h>
#include <stdint.h>
#include <peripherals/gentimer.h>
#include <peripherals/property_tags.h>
#include <peripherals/emmc.h>
#include <memory_manager.h>
#include <filesystem/mbr.h>
#include <filesystem/fat.h>
#include <filesystem/entrycache.h>
#include <peripherals/interrupt_controller.h>
#include <peripherals/system_timer.h>
#include <process/process.h>
#include <process/elf.h>
#include "peripherals/uart.h"
#include "page_mapping.h"
#include "printf.h"
#include "utils.h"
#include "filesystem/fatfs.h"
#include "interrupts/int_ctrl.h"
memory_manager memoryManager;
interrupt_controller interruptController;
ProcessManager* processManager;
EntryHashtable entryHashtable;
Filesystem* filesystem;



// Needed for printf
void _putchar(char character) {
    uart_putc(character);
}

void onTick() {
    processManager->getCurrentProcess()->processorTimeCounter--;
    if (processManager->getCurrentProcess()->processorTimeCounter > 0) {
        return;
    }
    processManager->schedule();
}

void runProcess(File* execFile) {
    auto elfFile = new elf_file(execFile);
    printf("Parse Result: %d", elfFile->parse());
    processManager->fork(elf_loader_fn, reinterpret_cast<u64>(elfFile));
    system_timer<1> timer;
    timer.setTickHandler(onTick);
    interruptController.register_handler(interrupt_controller::SYSTEM_TIMER_1, timer.getHandler());
    interruptController.enable(interrupt_controller::SYSTEM_TIMER_1);
    // Every millisecond
    timer.set_interval(1000);
    while(true) {
        bad_udelay(10000000);
        printf("Kernel Noot\n");
    }
}

void test_filesystem(FATFilesystem* fs, EntryHashtable* hashtable) {
    auto root = fs->getRootInfo();
    {
        auto file = root->lookup("HELLO.TXT");
        if (file.empty()) {
            printf("HELLO.TXT not found\n");
            return;
        }
        // Open file
        auto fp = fs->open(file);

        char text[100] = {0,};
        fp->read(text, file->size, 0);
        printf("File Contents: %s\n", text);
        strcat(text, "And goodbye.");
        // Not including null terminator
        auto x = strlen(text);
        fp->resize(x);
        fp->write(text, x, 0);
        fp->close();

        // TODO: Work on this
        delete fp;
    }
    printf("Cleaning\n");
    hashtable->clean();
    // Test reading again
    {
        auto fentry = root->lookup("HELLO.TXT");
        auto fp = fs->open(fentry);
        char text[100] = {0,};
        fp->read(text, fentry->size, 0);
        printf("File Contents: %s\n", text);
        fp->close();
        delete fp;
    }

    printf("Cleaning twice\n");
    hashtable->clean();
    hashtable->clean();
}

extern "C" /* Use C linkage for kernel_main. */
void kernel_main(uint32_t el, uint32_t r1, uint32_t atags)
{
    // Declare as unused
    (void) el;
    (void) r1;
    (void) atags;

    uart_init();
    // Test uart
    uart_puts("Hello, kernel World!\r\n");

    // Enable page mapping and malloc
    memoryManager = memory_manager();
    memoryManager.reserve_pages(KERNEL_START, KERNEL_SIZE/4096, PAGE_KERNEL);

    // Allow 'error handling'
    interruptController = interrupt_controller();
    processManager = new ProcessManager;
    set_vector_table();
    enable_interrupts();

    // Perform SD Card excitements
    SDCard my_sd;

    if (!my_sd.init()) {
        printf("SD Init Failure\n");
        return;
    }
    printf("SD Init Success\n");

    // Read first sector
    uint32_t first_sector[128];
    printf("first_sector size: %u\n", sizeof(first_sector));

    my_sd.read(0, first_sector, 512);
    printf("Did a read...\n");
    printf("Last 4 bytes: %u\n", first_sector[127]);
    printf("Partitions:\n");
    master_boot_record masterBootRecord(first_sector, &my_sd);
    entryHashtable = EntryHashtable();
    if (masterBootRecord.get_partition_info(0)->type != PART_FAT32) {
        printf("Mysterious Partition Found...\n");
        return;
    }

    partition* part = masterBootRecord.get_partition(0);

    // Initialise FAT
    FATFilesystem fs(part, &entryHashtable);
    fs = &fs;

    auto root = fs->getRootInfo();
    auto execEntry = root->lookup("EXEC1");
    auto execFile = fs->open(execEntry);
    runProcess(execFile);

    printf("Done\n");

    unsigned char c;
    // Infini-loop
    while(true) {
        c = uart_getc();
        bad_udelay(2*1000*1000); // Delay 2 seconds
        uart_putc(c);
    }



}