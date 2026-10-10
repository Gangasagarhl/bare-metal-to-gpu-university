/* entry.S - Multiboot (version 1) header and 32-bit entry of the CI test kernel.
   QEMU's -kernel option loads such a kernel and passes -append as the command line.
   Header values written from memory of the Multiboot Specification (pending verification);
   the lab run shows QEMU accepting the header and passing the command line. */
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
    call kmain
1:  cli
    hlt
    jmp 1b

    .bss
    .align 16
    .space 16384
stack_top:
