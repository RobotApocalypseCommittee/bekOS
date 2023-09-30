
.macro asm_arm64_dc_region op, start, end, linesize
1:  DC      \op, \start
    ADD     \start, \start, \linesize
    CMP     \start, \end
    B.LO    1b
    DSB     SY
.endm

.globl asm_arm64_clean_cache
asm_arm64_clean_cache:
    asm_arm64_dc_region CVAC, x0, x1, x2
    RET

.globl asm_arm64_invalidate_cache
asm_arm64_invalidate_cache:
    asm_arm64_dc_region IVAC, x0, x1, x2
    RET
