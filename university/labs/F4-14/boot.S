/* boot.S - DR401 lab kernel entry (Multiboot version 1, 32-bit protected mode, paging off).
 * QEMU's -kernel loader accepts a 32-bit ELF carrying this header. The kernel installs its own
 * flat GDT (code 0x08, data 0x10) and calls kmain(magic, info). With paging off, an address is a
 * physical address: device registers placed below 4 GiB by the firmware are used directly. */
        .set MB_MAGIC, 0x1BADB002
        .set MB_FLAGS, 0x00000003
        .section .multiboot, "a"
        .balign 4
        .long MB_MAGIC
        .long MB_FLAGS
        .long -(MB_MAGIC + MB_FLAGS)

        .section .text
        .globl _start
_start:
        cli
        lgdt gdt_ptr
        ljmp $0x08, $1f
1:      mov $0x10, %cx
        mov %cx, %ds
        mov %cx, %es
        mov %cx, %fs
        mov %cx, %gs
        mov %cx, %ss
        mov $stack_top, %esp
        push %ebx
        push %eax
        call kmain
2:      cli
        hlt
        jmp 2b

        .section .rodata
        .balign 8
gdt:    .quad 0
        .quad 0x00CF9A000000FFFF
        .quad 0x00CF92000000FFFF
gdt_ptr:
        .word gdt_ptr - gdt - 1
        .long gdt

        .section .bss
        .balign 16
        .skip 65536
stack_top:
        .section .note.GNU-stack, "", @progbits
