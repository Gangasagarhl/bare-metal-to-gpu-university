/* boot.S - F3-31 mini kernel: Multiboot (version 1) entry in 32-bit protected mode.
 * QEMU's -kernel loader accepts a 32-bit ELF with this header, loads the modules given
 * with -initrd, and jumps to _start with EAX = 0x2BADB002 and EBX = the physical address
 * of the Multiboot information structure (Multiboot Specification 0.6.96, section 3).
 * Paging stays off: in this teaching kernel a virtual address equals the physical one. */
        .set MB_MAGIC, 0x1BADB002
        .set MB_FLAGS, 0x00000003          /* bit 0: page-align modules; bit 1: memory info */

        .section .multiboot, "a"
        .balign 4
        .long MB_MAGIC
        .long MB_FLAGS
        .long -(MB_MAGIC + MB_FLAGS)       /* the three fields sum to 0 */

        .section .text
        .globl _start
_start:
        cli
        mov $stack_top, %esp
        push %ebx                          /* argument 2: Multiboot information */
        push %eax                          /* argument 1: magic value */
        call kmain
1:      hlt
        jmp 1b

        .section .bss
        .balign 16
        .skip 32768
stack_top:

        .section .note.GNU-stack, "", @progbits    /* no executable stack wanted */
