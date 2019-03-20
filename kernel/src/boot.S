// Aarch64
#include "system_registers.h"
#include "mm.h"

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

    // Move to EL1 - OS exec level
    // Disable MMU, and set system control
    ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

    // Hypervisor(EL2) state
	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

    // EL3 Configuration
	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

    // Saved status for EL3 -> El1
	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0

    // Store the address of the next code to run in the link register
    // This register is used when returning from exception
	adr	x0, master
	msr	elr_el3, x0
    // Return from 'exception'
	eret

master:
    // Clear bss section to 0, get start and length from linker
    ldr     x0, =__bss_start
    ldr     x1, =__bss_size
    bl memzero

    // set stack to start of code cos it grows downwards
    ldr     x1, =_start
    mov     sp, x1

    // get current Exception level
    mrs x0, CurrentEL
    lsr x0, x0, #2
4:  // going to c
    bl      kernel_main
    // stop this core
    b 1b
