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
ProcessManager processManager;

// Needed for printf
void _putchar(char character) {
    uart_putc(character);
}

void onTick() {
    processManager.getCurrentProcess()->processorTimeCounter--;
    if (processManager.getCurrentProcess()->processorTimeCounter > 0) {
        return;
    }
    processManager.schedule();
}

void nootProcess(u64 x) {
    while(1) {
        bad_udelay(500000);
        printf("Noot %d\n", x);
    }
}

void zootProcess(u64 x) {
    while(1) {
        bad_udelay(500000);
        printf("Zoot %d\n", x);
    }
}

extern "C" /* Use C linkage for kernel_main. */
void kernel_main(uint32_t el, uint32_t r1, uint32_t atags)
{
    // Declare as unused
    (void) r1;
    (void) atags;

    uart_init();
    uart_puts("Hello, kernel World!\r\n");
    memoryManager = memory_manager();
    interruptController = interrupt_controller();
    processManager = ProcessManager();
    processManager.fork(zootProcess, 10);
    processManager.fork(nootProcess, 11);


    system_timer<1> timer1;
    timer1.setTickHandler(onTick);
    interruptController.register_handler(interrupt_controller::SYSTEM_TIMER_1, timer1.getHandler());
    printf("Beginning Process Test\n");
    set_vector_table();
    enable_interrupts();
    interruptController.enable(interrupt_controller::SYSTEM_TIMER_1);
    timer1.set_interval(1000);
    printf("Prepare for switches\n");
    unsigned long rval;
    while (true) {

        bad_udelay(1000000);
        asm volatile ("mrs %0, CNTP_CTL_EL0": "=r" (rval));
        printf("rval %d, otherval %d\n", rval, mmio_read(PERIPHERAL_BASE + 0xB200));
    }
    set_vector_table();
    enable_interrupts();
    memoryManager.reserve_pages(KERNEL_START, KERNEL_SIZE/4096, PAGE_KERNEL);
    translation_table* table = new translation_table(&memoryManager);
    table->map(0, 0x1024);



    printf("The current exception level is %u\n", el);
    printf("General Timer Frequency: %u\n", getClockFrequency());

    PropertyTagClockRate clockTag;
    clockTag.clock_id = 0x000000001;
    property_tags tags;
    bool x = tags.request_tag(0x00030002, &clockTag, sizeof(PropertyTagClockRate));
    if (x) {
        printf("EMMC Clock Rate: %u\n", clockTag.rate);
    }

    clockTag.clock_id = 0x000000006;
    x = tags.request_tag(0x00030002, &clockTag, sizeof(PropertyTagClockRate));
    if (x) {
        printf("H264 Clock Rate: %u\n", clockTag.rate);
    }

    uint32_t cap0 = mmio_read(PERIPHERAL_BASE + 0x300000 + 0x40);
    bool supports_sdma = ((cap0>>22) & 1);
    if (supports_sdma) {
        printf("Supports SDMA\n");
    } else {
        printf("No SDMA\n");
    }
    printf("CAP0: %u\n", cap0);
    printf("Reported clock: %u\n", ((cap0 >> 8) & 0xff) * 1000000);

    printf("Here goes...\n");
    SDCard my_sd;
    if (my_sd.init()) {
        printf("Init success\n");
        // Declare a buffer array
        uint32_t my_array[128];
        printf("my_array size: %u\n", sizeof(my_array));
        my_sd.read(0, my_array, 512);
        printf("Did a read...\n");
        printf("Last 4 bytes: %u\n", my_array[127]);
        printf("Partitions:\n");
        master_boot_record masterBootRecord(my_array, &my_sd);
        EntryHashtable hashtable;
        if (masterBootRecord.get_partition_info(0)->type == PART_FAT32) {
            partition* noot = masterBootRecord.get_partition(0);

            FATFilesystem filesystem(noot, &hashtable);
            auto root = filesystem.getRootInfo();
            auto execFile = root->lookup("MEXEC");
            auto fp = filesystem.open(execFile);
            Process p;
            auto elf = elf_file(fp);
            printf("Parse Result: %d", elf.parse());
            elf.load(&p);

            fp->close();
            delete fp;
        }
        printf("Cleaning\n");
        hashtable.clean();
        printf("Cleaning twice\n");
        hashtable.clean();
        /*
        printf("Doing write\n");
        my_array[100] = 0x12345678;
        my_sd.write(0, my_array, 512);
         */
        printf("Done\n");
    } else {
        printf("Init not success\n");
    }

    unsigned char c;

    while(true) {
        c = uart_getc();
        bad_udelay(2*1000*1000); // Delay 2 seconds
        uart_putc(c);
    }



}