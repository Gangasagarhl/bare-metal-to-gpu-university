/* entry_x86.S - BR-06: x86 side of the boot contract (QEMU q35, -kernel with Multiboot).
 * QEMU's Multiboot loader enters 32-bit protected mode with paging off, eax = 0x2BADB002
 * and ebx = address of the Multiboot information structure (Multiboot Specification,
 * title only, pending verification; the lab run prints both). */
        .section .multiboot, "a"
        .balign 4
        .long   0x1BADB002                  /* header magic */
        .long   0x00000000                  /* flags: we ask for nothing special */
        .long   -(0x1BADB002 + 0x00000000)  /* checksum: the three fields sum to 0 */

        .text
        .code32
        .globl  _start
_start:
        mov     $__stack_top, %esp
        mov     %cs, %ecx                   /* low two bits of CS = current privilege level */
        mov     %cr0, %edx                  /* bit 0 = protected mode, bit 31 = paging */
        push    %edx
        push    %ecx
        push    %ebx                        /* Multiboot information address */
        push    %eax                        /* Multiboot magic */
        call    arch_entry                  /* arch_entry(eax, ebx, cs, cr0) */
1:      hlt
        jmp     1b

        .section .note.GNU-stack, "", %progbits
