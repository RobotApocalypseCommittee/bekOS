// clang-format off
#include "arch/a64/asm_defines.h"

// This function takes fn and arg from x19, x20 (saved regs) and calls it.
// fn should never return, but if it does we brk and load nullptr.
ASM_FUNCTION_BEGIN(kfunction_stub_a64)
    mov x0, x20
    blr x19
    brk #0
    mov x8, xzr
    ldr x0, [x8]
ASM_FUNCTION_END(kfunction_stub_a64)


ASM_FUNCTION_BEGIN(set_usertable)
    msr	ttbr0_el1, x0
    tlbi vmalle1is // clear caches
    DSB ISH // wait for data to be done
    isb // wait for instruction to be done
    ret
ASM_FUNCTION_END(set_usertable)


// userspace_first_entry(uPtr user_entry_point).
// This function is the entry (kernel) fn for a userspace process ready to start.
// It never returns; it does not use stack.
// We set the necessary return registers, zero-out the scratch registers, and eret.
ASM_FUNCTION_BEGIN(userspace_first_entry)
    bl disable_interrupts   // Interrupts at this point could be disasterous
    msr ELR_EL1, x0     // Return to provided entry point
    mov x1, xzr
    msr SPSR_EL1, x1    // Set to 0 - this reenables interrupts, etc. on eret
                        // Take it on trust that SP_EL0 set correctly

    mov x0, xzr         // Zero out GP registers
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    mov x8, xzr
    mov x9, xzr
    mov x10, xzr
    mov x11, xzr
    mov x12, xzr
    mov x13, xzr
    mov x14, xzr
    mov x15, xzr
    mov x16, xzr
    mov x17, xzr
    mov x18, xzr

    mov x19, xzr        // These two might have been set in executing kfunction_stub_a64
    mov x20, xzr
    eret
ASM_FUNCTION_END(userspace_first_entry)

// Called _by_ function we wish to return to, in kernel
// (x30 is populated, and we can change temporary registers)
ASM_FUNCTION_BEGIN(do_context_switch)
    // We need to store the stack pointer
    // And the el0 stack pointer
    mov x9, sp
    mrs x10, SP_EL0
    stp x19, x20, [x0], #16
    stp x21, x22, [x0], #16
    stp x23, x24, [x0], #16
    stp x25, x26, [x0], #16
    stp x27, x28, [x0], #16
    stp x29, x9, [x0], #16
    stp x30, x10, [x0]

    // Now restore the old ones
    ldp x19, x20, [x1], #16
    ldp x21, x22, [x1], #16
    ldp x23, x24, [x1], #16
    ldp x25, x26, [x1], #16
    ldp x27, x28, [x1], #16
    ldp x29, x9, [x1], #16
    ldp x30, x10, [x1]
    // Restore stack pointer
    mov sp, x9
    // Restore el0 stack pointer
    msr SP_EL0, x10
    // Return (to the new x30)
    ret
ASM_FUNCTION_END(do_context_switch)
