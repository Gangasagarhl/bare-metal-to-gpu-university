/* boot.S: entry point of the lab kernel (copied from F2-46 via F2-49; 32-bit x86, Multiboot).
   The Multiboot header values below are written from memory and are NOT verified against
   the Multiboot specification in this build (see the chapter's unverified box); the lab run
   shows only that QEMU accepted them. */
    .section .multiboot, "a"
    .align 4
    .long 0x1BADB002                /* magic */
    .long 0x00000000                /* flags: none requested */
    .long -(0x1BADB002 + 0x0)       /* checksum: magic + flags + checksum = 0 */

    .section .text
    .global _start
_start:
    mov $stack_top, %esp            /* our own stack, reserved in .bss below */
    /* Zero .bss using the boundaries the linker script exports. */
    mov $__bss_start, %edi
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    call kmain
1:  hlt
    jmp 1b

    .section .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top:
