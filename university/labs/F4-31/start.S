// start.S - DR403 kernel entry (AArch64). Shared by the kernels of F4-31 to F4-37.
//
// Contract assumed at entry (after the Linux "arm64 booting" document, title only - pending
// verification; QEMU's direct kernel boot follows it): x0 = physical address of the DTB,
// MMU off, at EL2 or EL1. The image is position independent: it is linked at address 0 and
// fixes its own absolute pointers (R_AARCH64_RELATIVE entries) wherever it was loaded, because
// the two platforms of F4-37 load it at different addresses.
    .section .text.head, "ax"
    .globl _start
_start:
    // 64-byte image header. QEMU recognises the magic and then loads the file at
    // (start of RAM + text_offset). Field layout after the arm64 booting document.
    b       entry               // code0: jump over the header
    .long   0                   // code1
    .quad   0x80000             // text_offset: load 512 KiB above the start of RAM
    .quad   _image_end - _start // image_size
    .quad   0xa                 // flags: little-endian, 4 KiB pages, anywhere in RAM
    .quad   0, 0, 0             // reserved
    .ascii  "ARM\x64"           // magic
    .long   0                   // reserved

entry:
    mov     x19, x0             // keep the DTB address
    mrs     x1, mpidr_el1       // only CPU 0 continues; any other CPU waits forever
    and     x1, x1, #0xff
    cbnz    x1, park
    mrs     x20, CurrentEL      // remember the level we were entered at (bits 3:2)
    lsr     x20, x20, #2
    cmp     x20, #2
    b.ne    at_el1
    // Entered at EL2: make EL1 AArch64, let EL1 use the physical counter, drop to EL1.
    mov     x1, #(1 << 31)      // HCR_EL2.RW = 1
    msr     hcr_el2, x1
    mov     x1, #3              // CNTHCTL_EL2: EL1PCTEN, EL1PCEN
    msr     cnthctl_el2, x1
    msr     cntvoff_el2, xzr
    mov     x1, #0x3c5          // SPSR: EL1h, interrupts masked
    msr     spsr_el2, x1
    adr     x1, at_el1
    msr     elr_el2, x1
    eret
at_el1:
    adrp    x1, __stack_top
    add     x1, x1, :lo12:__stack_top
    mov     sp, x1
    adrp    x1, __bss_start     // clear .bss (the loader does not)
    add     x1, x1, :lo12:__bss_start
    adrp    x2, __bss_end
    add     x2, x2, :lo12:__bss_end
1:  cmp     x1, x2
    b.hs    2f
    str     xzr, [x1], #8
    b       1b
2:  adr     x3, _start          // x3 = where we really are (the link address is 0)
    adrp    x1, __rela_start
    add     x1, x1, :lo12:__rela_start
    adrp    x2, __rela_end
    add     x2, x2, :lo12:__rela_end
3:  cmp     x1, x2              // each Elf64_Rela: offset, info, addend (24 bytes)
    b.hs    4f
    ldp     x4, x5, [x1], #16
    ldr     x6, [x1], #8
    cmp     w5, #1027           // R_AARCH64_RELATIVE
    b.ne    3b
    add     x6, x6, x3          // value  = load address + addend
    str     x6, [x4, x3]        // stored at load address + offset
    b       3b
4:  adrp    x1, vectors
    add     x1, x1, :lo12:vectors
    msr     vbar_el1, x1
    isb
    mov     x0, x19             // kmain(dtb, entry_el, load_address)
    mov     x1, x20
    mov     x2, x3
    bl      kmain
park:
    wfe
    b       park

// Exception vectors: 16 entries of 128 bytes; every entry reports and stops (no recovery).
    .balign 2048
vectors:
    .irp    n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    .balign 128
    mov     x0, #\n
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, far_el1
    b       exception_report
    .endr
