#include <mm/page_table.h>
#include <peripherals/gpio.h>

extern unsigned long __page_table_start;
extern unsigned long __page_table_end;

#define PGT_START ((unsigned long)&__page_table_start)
#define PGT_END ((unsigned long)&__page_table_end)
#define PGT_SIZE (PGT_END - PGT_START)

void blink(int n) {
    GPIO gpio(GPIO_BASE);
    for (int i = 0; i < n; i++) {
        gpio.set_pin(true, 29);
        delay(0x800000);
        gpio.set_pin(false, 29);
        delay(0x800000);
    }
}

extern "C"
void kernel_boot() {
    /// Warning: Everything done here MUST be lower-address friendly.
    /// No pointers can carry over to the 2nd stage of booting.

    GPIO gpio(mapped_address(GPIO_BASE));
    gpio.set_pin_function(Output, 29);
    gpio.set_pin(false, 29);

    mem_region page_table_region{.start=PGT_START, .length=PGT_SIZE};
    basic_translation_table table(page_table_region);

    auto res = table.map_region(0, 0, PERIPHERAL_OFFSET, AF, 0);
    if (!res) {
        blink(1);
        return;
    }
    res = table.map_region(PERIPHERAL_OFFSET, PERIPHERAL_OFFSET, ADDRESSABLE_MEMORY - PERIPHERAL_OFFSET, AF, 1);
    if (!res) {
        blink(2);
        return;
    }

    u64 r, b;
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    b=r&0xF;
    if(r&(0xF<<28)/*4k*//*36 bits*/) {
        blink(3);
        return;
    }

    // first, set Memory Attributes array
    r=  (0x44 << 0) |    // AttrIdx=0: normal, non cacheable
        (0x00 << 8);    // AttrIdx=1: device, nGnRnE
    asm volatile ("msr mair_el1, %0" : : "r" (r));

    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (16LL   << 16) | // T1SZ=16, 4 levels, 48 bits
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (16LL   << 0);   // T0SZ=16, 4 levels, 48 bits
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" (PGT_START | 1));
    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" (PGT_START | 1));

    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check

    // When we return, the enable bit will be set.
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));

}

