// Aarch64

// To ensure it's at the start
.section ".text.boot"

// Makes it global
.globl _start

    // Entry point for the pi
    .org 0x80000

// Kernel entry point
_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0 so stop
1:  wfe
    b       1b
2:  // cpu id == 0

    // set stack
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    // Calculating size??
    ldr     x2, =__bss_end
    sub     x2, x2, x1
3:  cbz     x2, 4f
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, 3b

4:  // going to c
    bl      kernel_main

    // stop this core
    b 1b
