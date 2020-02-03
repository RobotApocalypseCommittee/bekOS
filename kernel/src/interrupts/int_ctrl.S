
.globl set_vector_table
set_vector_table:
    adr    x0, irq_vectors       // load VBAR_EL1 with virtual
    msr    vbar_el1, x0        // vector table address
    ret

.globl enable_interrupts
enable_interrupts:
    msr    daifclr, #2
    ret