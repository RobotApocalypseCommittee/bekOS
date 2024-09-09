// clang-format off
#include "arch/a64/asm_defines.h"
#include "arch/a64/kernel_entry.h"

// Clobbers x9, x10
.macro save_context ctx
    stp x19, x20, [\ctx, #16 * 0]
    stp x21, x22, [\ctx, #16 * 1]
    stp x23, x24, [\ctx, #16 * 2]
    stp x25, x26, [\ctx, #16 * 3]
    stp x27, x28, [\ctx, #16 * 4]
    stp x29, x30, [\ctx, #16 * 5]

    mov x9, sp
    mrs x10, SP_EL0
    stp x9, x10, [\ctx, #16 * 6]
.endm

// Clobbers x9, x10
.macro restore_context ctx, pc_reg
    ldp x19, x20, [\ctx, #16 * 0]
    ldp x21, x22, [\ctx, #16 * 1]
    ldp x23, x24, [\ctx, #16 * 2]
    ldp x25, x26, [\ctx, #16 * 3]
    ldp x27, x28, [\ctx, #16 * 4]
    ldp x29, \pc_reg, [\ctx, #16 * 5]
    ldp x9, x10, [\ctx, #16 * 6]
    mov sp, x9
    msr SP_EL0, x10
.endm

// process_entry.h: void do_context_switch(SavedRegisters& previous, SavedRegisters& next)
// Called _by_ function we wish to return to, in kernel
// (x30 is populated, and we can change temporary registers)
ASM_FUNCTION_BEGIN(do_context_switch)
    // Will save x30 to pc/lr slot.
    save_context x0
    // Restore pc/lr slot to x30.
    restore_context x1, x30
    // Jump to restored pc/lr as return.
    ret
ASM_FUNCTION_END(do_context_switch)

// process_entry.h: void do_switch_user_address_space(uPtr new_address_root)
ASM_FUNCTION_BEGIN(do_switch_user_address_space)
    msr	ttbr0_el1, x0
    tlbi vmalle1is // clear caches
    DSB ISH // wait for data to be done
    isb // wait for instruction to be done
    ret
ASM_FUNCTION_END(do_switch_user_address_space)

// process_entry.h: uPtr do_get_current_user_stack()
ASM_FUNCTION_BEGIN(do_get_current_user_stack)
    mrs x0, sp_el0
    ret
ASM_FUNCTION_END(do_get_current_user_stack)

// process_entry.h: [[noreturn]] void do_assume_process_state(SavedRegisters& registers, u64 argument)
ASM_FUNCTION_BEGIN(do_assume_process_state)
    // Restores provided context; x2 is function to jump to.
    restore_context x0, x2
    inline_enable_interrupts
    mov x0, x1
    br x2
ASM_FUNCTION_END(do_assume_process_state)


// saved_registers.cpp: [[noreturn]] kernel_first_entry_a64()
// x19 = fn, x20 = arg.
ASM_FUNCTION_BEGIN(kernel_first_entry_a64)
    mov x0, x20
    br x19
ASM_FUNCTION_END(kernel_first_entry_a64)

// saved_registers.cpp: [[noreturn]] user_first_entry_a64()
// x19 = user_entry.
ASM_FUNCTION_BEGIN(user_first_entry_a64)
    inline_disable_interrupts
    msr ELR_EL1, x19
    mov x0, xzr
    msr SPSR_EL1, x0    // Reset state.

    mov x1, xzr         // Zero out GP registers
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
    mov x19, xzr

    eret
ASM_FUNCTION_END(user_first_entry_a64)


// saved_registers.cpp: [[noreturn]] ret_from_fork_a64()
// An InterruptContext has been pushed onto otherwise brand new stack.
// We take no arguments - everything that should be 0 is 0.
ASM_FUNCTION_BEGIN(ret_from_fork_a64)
    inline_disable_interrupts   // Interrupts at this point could be disasterous
    restore_regs
    eret
ASM_FUNCTION_END(ret_from_fork_a64)



