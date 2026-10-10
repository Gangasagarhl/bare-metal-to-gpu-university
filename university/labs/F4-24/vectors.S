/* vectors.S - F4-24: the EL1 exception vector table. VBAR_EL1 points here (boot.S).
 * 16 entries of 0x80 bytes, table aligned to 2 KiB: four groups (current EL with SP_EL0,
 * current EL with SP_ELx, lower EL AArch64, lower EL AArch32), each with the four kinds
 * synchronous, IRQ, FIQ, SError. Layout after the Arm Architecture Reference Manual,
 * exception model chapter (title only, pending verification).
 * Every entry saves a TrapFrame (trap.h) and calls exception_entry(frame, index). */
        .set FRAME_SIZE, 288                /* 31 registers + elr, spsr, esr, far = 35 x 8, rounded to 16 */

        .macro ventry index
        .balign 0x80
        sub     sp, sp, #FRAME_SIZE
        stp     x0, x1, [sp, #0]
        mov     x0, #\index
        b       exception_common
        .endm

        .text
        .balign 0x800
        .globl vector_table
vector_table:
        ventry 0                            /* current EL, SP_EL0: sync */
        ventry 1                            /*                     IRQ  */
        ventry 2                            /*                     FIQ  */
        ventry 3                            /*                     SError */
        ventry 4                            /* current EL, SP_ELx: sync  (kernel faults) */
        ventry 5                            /*                     IRQ   (F4-25 timer) */
        ventry 6                            /*                     FIQ  */
        ventry 7                            /*                     SError */
        ventry 8                            /* lower EL, AArch64: sync (system calls later) */
        ventry 9
        ventry 10
        ventry 11
        ventry 12                           /* lower EL, AArch32 */
        ventry 13
        ventry 14
        ventry 15

exception_common:
        stp     x2, x3, [sp, #16]
        stp     x4, x5, [sp, #32]
        stp     x6, x7, [sp, #48]
        stp     x8, x9, [sp, #64]
        stp     x10, x11, [sp, #80]
        stp     x12, x13, [sp, #96]
        stp     x14, x15, [sp, #112]
        stp     x16, x17, [sp, #128]
        stp     x18, x19, [sp, #144]
        stp     x20, x21, [sp, #160]
        stp     x22, x23, [sp, #176]
        stp     x24, x25, [sp, #192]
        stp     x26, x27, [sp, #208]
        stp     x28, x29, [sp, #224]
        mrs     x2, elr_el1
        stp     x30, x2, [sp, #240]         /* x30 at 240, elr at 248 */
        mrs     x2, spsr_el1
        mrs     x3, esr_el1
        stp     x2, x3, [sp, #256]          /* spsr at 256, esr at 264 */
        mrs     x2, far_el1
        str     x2, [sp, #272]              /* far at 272 */
        mov     x1, x0                      /* exception_entry(frame, index) */
        mov     x0, sp
        bl      exception_entry
        ldp     x2, x3, [sp, #256]          /* the handler may change elr and spsr */
        msr     spsr_el1, x2
        ldr     x2, [sp, #248]
        msr     elr_el1, x2
        ldp     x0, x1, [sp, #0]
        ldp     x2, x3, [sp, #16]
        ldp     x4, x5, [sp, #32]
        ldp     x6, x7, [sp, #48]
        ldp     x8, x9, [sp, #64]
        ldp     x10, x11, [sp, #80]
        ldp     x12, x13, [sp, #96]
        ldp     x14, x15, [sp, #112]
        ldp     x16, x17, [sp, #128]
        ldp     x18, x19, [sp, #144]
        ldp     x20, x21, [sp, #160]
        ldp     x22, x23, [sp, #176]
        ldp     x24, x25, [sp, #192]
        ldp     x26, x27, [sp, #208]
        ldp     x28, x29, [sp, #224]
        ldr     x30, [sp, #240]
        add     sp, sp, #FRAME_SIZE
        eret
