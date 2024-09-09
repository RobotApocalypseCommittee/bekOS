// clang-format off
#include "arch/a64/asm_defines.h"

.macro asm_arm64_dc_region op, start, end, linesize
1:  dc      \op, \start
    add     \start, \start, \linesize
    cmp     \start, \end
    b.lo    1b
    dsb     SY
.endm

ASM_FUNCTION_BEGIN(asm_arm64_clean_cache)
    asm_arm64_dc_region CVAC, x0, x1, x2
    ret
ASM_FUNCTION_END(asm_arm64_clean_cache)

ASM_FUNCTION_BEGIN(asm_arm64_invalidate_cache)
    asm_arm64_dc_region IVAC, x0, x1, x2
    ret
ASM_FUNCTION_END(asm_arm64_invalidate_cache)

ASM_FUNCTION_BEGIN(do_set_vector_table)
    adr    x0, irq_vectors       // load VBAR_EL1 with virtual
    msr    vbar_el1, x0        // vector table address
    ret
ASM_FUNCTION_END(do_set_vector_table)

ASM_FUNCTION_BEGIN(enable_interrupts)
    msr    daifclr, #2
    ret
ASM_FUNCTION_END(enable_interrupts)

ASM_FUNCTION_BEGIN(disable_interrupts)
    msr    daifset, #2
    ret
ASM_FUNCTION_END(disable_interrupts)