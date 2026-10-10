/* start.S - BR-07: entry of the small AArch64 program for QEMU's raspi3b model.
 * Same shape as F4-24's boot.S (DR402): a 64-byte arm64 image header, keep x0 (the devicetree
 * address), drop from EL2 to EL1, set a stack and a vector table, clear .bss, call kmain.
 * Header layout after the Linux document "Booting AArch64 Linux" (title only, pending
 * verification); QEMU accepting the file and passing x0 is what the lab run shows. */
        .section .head.text, "ax"
        .globl _start
_start:
        b       entry                       /* code0: branch over the header */
        .long   0
        .quad   0x80000                     /* text_offset */
        .quad   __image_size
        .quad   0x2                         /* little-endian, 4 KiB pages */
        .quad   0, 0, 0
        .ascii  "ARM\x64"                   /* magic */
        .long   0

        .text
entry:
        mov     x19, x0                     /* devicetree address from the loader */
        mrs     x1, mpidr_el1               /* only CPU 0 runs this program */
        and     x1, x1, #0xff
        cbnz    x1, park
        mrs     x1, CurrentEL
        lsr     x1, x1, #2
        cmp     x1, #2
        b.ne    at_el1
        mov     x1, #(1 << 31)              /* HCR_EL2.RW: EL1 is AArch64 */
        msr     hcr_el2, x1
        mov     x1, #0x3c5                  /* EL1h, interrupts masked */
        msr     spsr_el2, x1
        adr     x1, at_el1
        msr     elr_el2, x1
        eret
at_el1:
        ldr     x1, =__stack_top
        mov     sp, x1
        ldr     x1, =vectors
        msr     vbar_el1, x1
        isb
        ldr     x1, =__bss_start
        ldr     x2, =__bss_end
1:      cmp     x1, x2
        b.hs    2f
        str     xzr, [x1], #8
        b       1b
2:      mov     x0, x19
        bl      kmain
park:   wfe
        b       park

/* 16 vector entries of 128 bytes; each one reports what happened and stops. */
        .balign 2048
vectors:
        .irp    n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
        .balign 128
        mov     x0, #\n
        mrs     x1, esr_el1
        mrs     x2, elr_el1
        mrs     x3, far_el1
        b       on_exception
        .endr
