/* boot.S - DR301 lab kernel: Multiboot (version 1) entry in 32-bit protected mode.
 * QEMU's -kernel loader accepts a 32-bit ELF with this header and jumps to _start with
 * EAX = 0x2BADB002 and EBX = the physical address of the Multiboot information structure.
 * The kernel loads its own flat GDT (code selector 0x08, data selector 0x10), because the
 * loader's GDT may not be relied on once the kernel installs an IDT (F4-03).
 * Paging stays off: in this teaching kernel a virtual address equals the physical one,
 * so device registers below 4 GiB and DMA buffers are used at their physical addresses. */
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
        lgdt gdt_ptr
        ljmp $0x08, $1f                    /* reload CS from our GDT */
1:      mov $0x10, %cx
        mov %cx, %ds
        mov %cx, %es
        mov %cx, %fs
        mov %cx, %gs
        mov %cx, %ss
        mov $stack_top, %esp
        push %ebx                          /* argument 2: Multiboot information */
        push %eax                          /* argument 1: magic value */
        call kmain
2:      cli
        hlt
        jmp 2b

        .section .rodata
        .balign 8
gdt:    .quad 0                            /* null descriptor */
        .quad 0x00CF9A000000FFFF           /* 0x08: code, base 0, limit 4 GiB, ring 0 */
        .quad 0x00CF92000000FFFF           /* 0x10: data, base 0, limit 4 GiB, ring 0 */
gdt_ptr:
        .word gdt_ptr - gdt - 1
        .long gdt

        .section .bss
        .balign 16
        .skip 65536
stack_top:

        .section .note.GNU-stack, "", @progbits    /* no executable stack wanted */
