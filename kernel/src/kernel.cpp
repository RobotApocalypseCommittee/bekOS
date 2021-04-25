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

#include <filesystem/entrycache.h>
#include <filesystem/fat.h>
#include <filesystem/mbr.h>
#include <memory_manager.h>
#include <peripherals/emmc.h>
#include <peripherals/framebuffer.h>
#include <peripherals/gentimer.h>
#include <peripherals/interrupt_controller.h>
#include <peripherals/property_tags.h>
#include <peripherals/system_timer.h>
#include <process/elf.h>
#include <process/process.h>
#include <usbborrowed/rpi-usb.h>

#include "filesystem/fatfs.h"
#include "interrupts/int_ctrl.h"
#include "page_mapping.h"
#include "peripherals/uart.h"
#include "printf.h"
#include "usb/DWHCI.h"
#include "utils.h"

extern "C" {

// ABI stuff
void* __dso_handle;

/// These are no-op for now - I'm assuming destructors don't need to run when system stops
int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso) {return 0;}
void __cxa_finalize(void *f) {}
}

memory_manager memoryManager;
interrupt_controller interruptController;
ProcessManager *processManager;
fs::EntryHashtable entryHashtable;
fs::Filesystem *filesystem;
char_blitter* charBlitter;

// Needed for printf
void _putchar(char character) {
    uart_putc(character);
    if (charBlitter) {
        charBlitter->putChar(character);
    }
}

void onTick() {
    processManager->getCurrentProcess()->processorTimeCounter--;
    if (processManager->getCurrentProcess()->processorTimeCounter > 0) {
        return;
    }
    processManager->schedule();
}

void runProcess(fs::File *execFile) {
    auto elfFile = new elf_file(execFile);
    printf("Parse Result: %d", elfFile->parse());
    processManager->fork(elf_loader_fn, reinterpret_cast<u64>(elfFile));
    system_timer<1> timer;
    timer.setTickHandler(onTick);
    interruptController.register_handler(interrupt_controller::SYSTEM_TIMER_1, timer.getHandler());
    interruptController.enable(interrupt_controller::SYSTEM_TIMER_1);
    // Every millisecond
    timer.set_interval(1000);
    while (true) {
        bad_udelay(10000000);
        printf("Kernel Noot\n");
    }
}

void test_filesystem(fs::FATFilesystem *fs, fs::EntryHashtable *hashtable) {
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
void kernel_main(u32 el, u32 r1, u32 atags)
{
    // Declare as unused
    (void) el;
    (void) r1;
    (void) atags;

    uart_init();

    colour black(0);
    colour white(255, 255, 255, 0);

    framebuffer_info fb_info = {
        .width = 640,
        .height = 480,
        .depth = 32
    };

    allocate_framebuffer(fb_info);
    framebuffer fb(fb_info);
    fb.clear(white);
    char_blitter cb(fb, white, black);
    charBlitter = &cb;
    // Test uart
    printf("Hello, kernel World!\r\n");

    // Enable page mapping and malloc
    memoryManager = memory_manager();
    memoryManager.reserve_pages(KERNEL_START, KERNEL_SIZE/4096, PAGE_KERNEL);

    // Allow 'error handling'
    interruptController = interrupt_controller();
    processManager = new ProcessManager;
    set_vector_table();
    enable_interrupts();

    /* Initialize USB system we will want keyboard and mouse */
    USB::UsbInitialise();

    /* Display the USB tree */
    printf("\n");
    UsbShowTree(USB::UsbGetRootHub(), 1, '+');
    printf("\n");

    /* Detect the first keyboard on USB bus */
    uint8_t firstKbd = 0;
    for (int i = 1; i <= MaximumDevices; i++) {
        if (USB::IsKeyboard(i)) {
            firstKbd = i;
            break;
        }
    }
    if (firstKbd) printf("Keyboard detected\r\n");

    /*
    // Perform SD Card excitements
    SDCard my_sd;

    if (!my_sd.init()) {
        printf("SD Init Failure\n");
        return;
    }
    printf("SD Init Success\n");

    printf("Partitions:\n");
    auto mbr = readMBR(my_sd);
    entryHashtable = fs::EntryHashtable();
    if (mbr.partitions[0].type != PART_FAT32) {
        printf("Mysterious Partition Found...\n");
        return;
    }

    auto *part = new Partition(&my_sd, mbr.partitions[0].start, mbr.partitions->size);

    // Initialise FAT
    fs::FATFilesystem my_fs(*part, entryHashtable);
    filesystem = &my_fs;

    auto root = my_fs.getRootInfo();
    auto execEntry = root->lookup("EXEC1");
    auto *execFile = my_fs.open(execEntry);
    runProcess(execFile);
*/
    printf("Done\n");



    unsigned char c;
    // Infini-loop
    while(true) {
        c = uart_getc();
        bad_udelay(2*1000*1000); // Delay 2 seconds
        uart_putc(c);
        cb.putChar(c);
    }
}