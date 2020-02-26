.globl process_begin
process_begin: blr x19

.globl do_context_switch
// Called _by_ function we wish to return to
// (x30 is populated, and we can change temporary registers)
do_context_switch:
    // We need to store the stack pointer
    mov x9, sp
    stp x19, x20, [x0], #16
    stp x21, x22, [x0], #16
    stp x23, x24, [x0], #16
    stp x25, x26, [x0], #16
    stp x27, x28, [x0], #16
    stp x29, x9, [x0], #16
    str x30, [x0]

    // Now restore the old ones
    ldp x19, x20, [x1], #16
    ldp x21, x22, [x1], #16
    ldp x23, x24, [x1], #16
    ldp x25, x26, [x1], #16
    ldp x27, x28, [x1], #16
    ldp x29, x9, [x1], #16
    ldr x30, [x1]
    // Restore stack pointer
    mov sp, x9
    // Return (to the new x30)
    ret