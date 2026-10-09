/* mb_entry.S - a Multiboot (version 1) kernel header and 32-bit entry point, for comparison
   with our own protocol. QEMU's -kernel option can load such a kernel directly.
   Header numbers written from memory of the Multiboot Specification (pending verification). */
    .set MB_MAGIC, 0x1BADB002          /* the header magic the loader searches for */
    .set MB_FLAGS, 0x00000003          /* bit 0: align modules; bit 1: give memory information */

    .section .multiboot, "a"
    .align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long -(MB_MAGIC + MB_FLAGS)       /* the three words must add up to zero */

    .text
    .code32
    .global _start
_start:                                /* 32-bit protected mode, paging off, interrupts off */
    mov $stack_top, %esp
    push %ebx                          /* physical address of the Multiboot information */
    push %eax                          /* the magic value the loader put in EAX */
    call mb_main
1:  hlt
    jmp 1b

    .bss
    .align 16
    .space 16384
stack_top:
