// el3_entry.S - F11-05: reset entry and exception vectors of our secure monitor (EL3).
// The CPU of QEMU's virt board with secure=on starts here, at address 0 (the secure flash),
// in the secure state at EL3.
    .section .text.boot, "ax"
    .global _start
_start:
    ldr     x0, =__el3_stack_top        // stack in secure RAM
    mov     sp, x0
    ldr     x0, =el3_vectors
    msr     vbar_el3, x0
    isb
    bl      el3_main                    // C++: set up, then returns the normal-world entry in x0
    // configure the lower exception level and leave EL3 for the normal world
#ifdef CONTROL_SECURE_EL1
    mov     x1, #(1 << 10)              // control run only: RW=1, NS=0 (EL1 stays SECURE)
#else
    mov     x1, #(1 << 10) | (1 << 0)   // SCR_EL3: RW=1 (EL1 is AArch64), NS=1 (non-secure)
#endif
    msr     scr_el3, x1
    mov     x1, #0x3c5                  // SPSR_EL3: EL1h (EL1, own stack), D A I F masked
    msr     spsr_el3, x1
    msr     elr_el3, x0                 // where ERET goes
    ldr     x1, =0x40300000             // normal-world stack top (normal RAM)
    msr     sp_el1, x1
    isb
    eret

// Vector table: 16 entries of 0x80 bytes; we use "lower EL, AArch64, synchronous" (0x400).
    .balign 0x800
el3_vectors:
    .rept 8
    .balign 0x80
    b       el3_unexpected
    .endr
    .balign 0x80                        // 0x400: synchronous exception from a lower EL (SMC)
    b       el3_smc_entry
    .rept 7
    .balign 0x80
    b       el3_unexpected
    .endr

el3_smc_entry:
    // save the normal world's general registers x0-x30 on the EL3 stack (34 slots, 16-aligned)
    sub     sp, sp, #(34 * 8)
    stp     x0, x1, [sp, #0]
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
    str     x30, [sp, #240]
    mov     x0, sp                      // el3_smc(uint64_t* regs): reads x0-x3, writes x0
    bl      el3_smc
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
    add     sp, sp, #(34 * 8)
    eret                                // back to the instruction after the SMC

el3_unexpected:
    bl      el3_panic
1:  wfe
    b       1b

// QEMU semihosting call (AArch64: HLT #0xF000), used only to end the run with an exit code.
    .global semihost_call
semihost_call:
    hlt     #0xf000
    ret
