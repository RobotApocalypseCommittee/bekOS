.section ".text.vec"

.macro complain_unknown_interrupt
    store_regs
    // Arguments
    mrs	x0, esr_el1
    mrs	x1, elr_el1
    bl show_unknown_int_complaint
1:  wfe
    b 1b
.endm


.macro store_regs
    sub	sp, sp, #160
    // Store General Purpose registers
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	// Store link register - we may want to bl
	stp x29, x30, [sp, #16 * 8]
	// Store the return registers
	// Read SPSR_EL1 and ELR_EL1 into GP registers
    MRS x0, SPSR_EL1
    MRS x1, ELR_EL1
    // Stack SPSR_EL1 and ELR_EL1
    STP x0, x1, [sp, #16 * 9]
.endm

.macro restore_regs
    // Restore exception regs
    LDP x0, x1, [sp, #16 * 9]
    MSR x0, SPSR_EL1
    MSR x1, ELR_EL1
    // Restore general regs
    LDP x0, x1, [sp, #16 * 0]
    LDP x2, x3, [sp, #16 * 1]
    LDP x4, x5, [sp, #16 * 2]
    LDP x6, x7, [sp, #16 * 3]
    LDP x8, x9, [sp, #16 * 4]
    LDP x10, x11, [sp, #16 * 5]
    LDP x12, x13, [sp, #16 * 6]
    LDP x14, x15, [sp, #16 * 7]
    LDP x29, x30  [sp, #16 * 8]
    add sp, sp, #INT_STACK_SIZE
.endm

.macro vector_entry branchlabel
.align 7
b \branchlabel
.endm

// VBAR has reserved 0 bottom 11 bits
.align 11
.globl irq_vectors
irq_vectors:
    vector_entry el1_s0_sync
    vector_entry el1_s0_irq
    vector_entry el1_s0_fiq
    vector_entry el1_s0_err

    vector_entry el1_s1_sync
    vector_entry el1_s1_irq
    vector_entry el1_s1_fiq
    vector_entry el1_s1_err

    vector_entry el0_64_sync
    vector_entry el0_64_irq
    vector_entry el0_64_fiq
    vector_entry el0_64_err

    vector_entry el0_32_sync
    vector_entry el0_32_irq
    vector_entry el0_32_fiq
    vector_entry el0_32_err

el1_s0_sync:
    complain_unknown_interrupt
el1_s0_irq:
    complain_unknown_interrupt
el1_s0_fiq:
    complain_unknown_interrupt
el1_s0_err:
    complain_unknown_interrupt
el1_s1_sync:
    complain_unknown_interrupt
el1_s1_irq:
    complain_unknown_interrupt
el1_s1_fiq:
    complain_unknown_interrupt
el1_s1_err:
    complain_unknown_interrupt
el0_64_sync:
    complain_unknown_interrupt
el0_64_irq:
    complain_unknown_interrupt
el0_64_fiq:
    complain_unknown_interrupt
el0_64_err:
    complain_unknown_interrupt
el0_32_sync:
    complain_unknown_interrupt
el0_32_irq:
    complain_unknown_interrupt
el0_32_fiq:
    complain_unknown_interrupt
el0_32_err:
    complain_unknown_interrupt
