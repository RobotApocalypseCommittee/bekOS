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
