// bl31_vec.S - F4-32: the EL3 vector table of the resident monitor and its hand-over to BL33.
    .text
    .balign 2048
    .globl el3_vectors
el3_vectors:
    .irp    n, 0,1,2,3,4,5,6,7
    .balign 128
    mov     x0, #\n
    b       el3_unexpected
    .endr
    .balign 128                    // 0x400: synchronous exception from a lower EL in AArch64
    b       el3_sync_lower
    .irp    n, 9,10,11,12,13,14,15
    .balign 128
    mov     x0, #\n
    b       el3_unexpected
    .endr

el3_sync_lower:                    // save what C++ may clobber, call smc_dispatch, return x0-x3
    sub     sp, sp, #(20 * 8)
    stp     x4, x5, [sp, #0]
    stp     x6, x7, [sp, #16]
    stp     x8, x9, [sp, #32]
    stp     x10, x11, [sp, #48]
    stp     x12, x13, [sp, #64]
    stp     x14, x15, [sp, #80]
    stp     x16, x17, [sp, #96]
    stp     x18, x29, [sp, #112]
    str     x30, [sp, #128]
    mrs     x4, esr_el3
    bl      smc_dispatch           // x0 = function id, x1-x3 = arguments, x4 = ESR_EL3
    ldp     x4, x5, [sp, #0]
    ldp     x6, x7, [sp, #16]
    ldp     x8, x9, [sp, #32]
    ldp     x10, x11, [sp, #48]
    ldp     x12, x13, [sp, #64]
    ldp     x14, x15, [sp, #80]
    ldp     x16, x17, [sp, #96]
    ldp     x18, x29, [sp, #112]
    ldr     x30, [sp, #128]
    add     sp, sp, #(20 * 8)
    eret                           // ELR_EL3 already points after the SMC instruction

el3_unexpected:
    mrs     x1, esr_el3
    mrs     x2, elr_el3
    b       el3_report

// enter_bl33(entry, dtb, scr, spsr): program EL3's return state and drop to the normal world.
    .globl enter_bl33
enter_bl33:
    msr     scr_el3, x2
    msr     spsr_el3, x3
    msr     elr_el3, x0
    mov     x0, x1                 // the kernel finds the DTB address in x0
    mov     x1, xzr
    mov     x2, xzr
    mov     x3, xzr
    isb
    eret
