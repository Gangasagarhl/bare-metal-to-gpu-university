/* entry_aarch64.S - BR-06: AArch64 side of the boot contract (QEMU virt, -kernel).
 * The 64-byte arm64 Image header makes QEMU treat the file as a Linux-style Image, so it
 * passes the devicetree address in x0 (header layout after "Booting AArch64 Linux",
 * title only, pending verification; the lab run shows what x0 really holds). */
        .section .head.text, "ax"
        .globl _start
_start:
        b       entry                       /* code0: branch over the header */
        .long   0                           /* code1 */
        .quad   0x80000                     /* text_offset: RAM base + 512 KiB */
        .quad   __image_size                /* image_size */
        .quad   0x2                         /* flags: little-endian, 4 KiB pages */
        .quad   0, 0, 0                     /* reserved */
        .ascii  "ARM\x64"                   /* magic */
        .long   0                           /* reserved */

        .text
entry:
        mrs     x4, CurrentEL               /* exception level, in bits 3:2 */
        ldr     x5, =__stack_top
        mov     sp, x5
        ldr     x5, =vectors                /* exceptions taken at EL1 go here ... */
        msr     vbar_el1, x5
        ldr     x5, =vectors                /* ... and at EL2, if we were started there */
        mrs     x6, CurrentEL
        cmp     x6, #8
        b.ne    1f
        msr     vbar_el2, x5
1:      isb
        bl      arch_entry                  /* arch_entry(x0, x1, x2, x3, CurrentEL) */
2:      wfe
        b       2b

/* 16 vector entries of 128 bytes each, table aligned to 2 KiB; every entry reports. */
        .balign 2048
vectors:
        .rept   16
        b       fatal
        .balign 128
        .endr
fatal:
        bl      arch_fatal                  /* reads the syndrome registers; never returns */
3:      b       3b

        .section .note.GNU-stack, "", %progbits
