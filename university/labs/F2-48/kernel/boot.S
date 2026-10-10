/* boot.S: entry point of the F2-48 kernel (32-bit x86, QEMU's Multiboot loader).
   Same as F2-46: the Multiboot header values are from memory (unverified box in F2-46). */
    .section .multiboot, "a"
    .align 4
    .long 0x1BADB002
    .long 0x00000000
    .long -(0x1BADB002 + 0x0)

    .section .text
    .global _start
_start:
    mov $stack_top, %esp
    mov $__bss_start, %edi          /* zero .bss */
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    call kmain
1:  hlt
    jmp 1b

    .section .bss
    .align 16
    .skip 16384
stack_top:
