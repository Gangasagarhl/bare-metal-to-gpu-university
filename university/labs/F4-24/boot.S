/* boot.S - F4-24: AArch64 entry for QEMU virt (and, untested, for boards that load a
 * Linux-style arm64 Image). The 64-byte header lets a loader treat the file as an arm64
 * Image: QEMU then passes the physical address of the devicetree blob in x0.
 * Header layout after the Linux kernel document "Booting AArch64 Linux"
 * (title only, pending verification); QEMU accepting the file is checked by the lab run. */
        .section .head.text, "ax"
        .globl _start
_start:
        b       primary_entry               /* code0: branch over the header */
        .long   0                           /* code1 */
        .quad   0x80000                     /* text_offset: load 512 KiB above the RAM base */
        .quad   __image_size                /* image_size: file image plus .bss */
        .quad   0x2                         /* flags: little-endian, 4 KiB pages (bits 1-2 = 01) */
        .quad   0, 0, 0                     /* reserved */
        .ascii  "ARM\x64"                   /* magic: 0x644d5241 */
        .long   0                           /* reserved (offset to a PE header for EFI stubs) */

        .text
primary_entry:
        mov     x19, x0                     /* x0 = devicetree blob address: keep it */
        mrs     x20, CurrentEL              /* bits 3:2 = exception level at entry */
        lsr     x20, x20, #2
        and     x20, x20, #3
        cmp     x20, #2
        b.ne    at_el1
#ifndef FORENSIC_STAY_AT_EL2               /* defined only for the forensic kernel */
        /* Entered at EL2 (QEMU -M virt,virtualization=on, or a board's firmware):
         * stay a plain EL1 kernel. EL1 runs AArch64, may use the physical timer and
         * counter, and eret drops to EL1h with every interrupt masked. */
        mov     x0, #(1 << 31)              /* HCR_EL2.RW = 1: EL1 is AArch64 */
        msr     hcr_el2, x0
        mov     x0, #3                      /* CNTHCTL_EL2: EL1PCTEN | EL1PCEN */
        msr     cnthctl_el2, x0
        msr     cntvoff_el2, xzr
        mov     x0, #0x3c5                  /* SPSR: D, A, I, F masked; mode EL1h */
        msr     spsr_el2, x0
        adr     x0, at_el1
        msr     elr_el2, x0
        eret
#endif
at_el1:
        msr     daifset, #0xf               /* no interrupts until the GIC is set up (F4-25) */
        ldr     x0, =__stack_top            /* boot CPU stack (linker.ld) */
        mov     sp, x0
        ldr     x0, =vector_table           /* VBAR_EL1: base of the 16-entry table */
        msr     vbar_el1, x0
        isb
        ldr     x0, =__bss_start            /* clear .bss before any C++ runs */
        ldr     x1, =__bss_end
1:      cmp     x0, x1
        b.hs    2f
        str     xzr, [x0], #8
        b       1b
2:      bl      run_global_constructors     /* F3-18 cxxrt.cc, unchanged */
        mov     x0, x19                     /* kmain(dtb, entry_el) */
        mov     x1, x20
        bl      kmain
3:      wfe                                 /* kmain does not return; if it does, sleep */
        b       3b
