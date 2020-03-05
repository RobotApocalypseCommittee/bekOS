.globl process_begin
process_begin:
    mov x0, x20
    blr x19

.globl become_userspace
become_userspace:
    bl disable_interrupts
    mov x2, #0
    msr SP_EL0, x1
    msr SPSR_EL1, x2
    msr ELR_EL1, x0
    eret

.globl do_context_switch
// Called _by_ function we wish to return to, in kernel
// (x30 is populated, and we can change temporary registers)
do_context_switch:
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