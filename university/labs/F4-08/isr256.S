/* isr256.S - DR302 F4-08: 256 interrupt entry stubs for the lab kernel (32-bit protected
 * mode). DR301's isr.S had 48 stubs for the 8259 PICs; message-signalled interrupts may
 * use any vector from 32 to 255, so DR302 installs a full IDT. Every stub is 16 bytes
 * apart; vectors with a CPU error code (8, 10-14, 17, per DR301 F4-03) push none. */
        .text
        .globl isr256_stubs
        .balign 16
isr256_stubs:
        .set vec, 0
        .rept 256
        .balign 16
        .if (vec == 8) || ((vec >= 10) && (vec <= 14)) || (vec == 17)
        .else
        pushl $0                            /* dummy error code */
        .endif
        pushl $vec                          /* vector number */
        jmp isr256_common
        .set vec, vec + 1
        .endr

isr256_common:
        pusha                               /* eax ecx edx ebx esp ebp esi edi */
        mov %esp, %eax
        push %eax                           /* argument: pointer to the frame */
        cld
        call intr_dispatch
        add $4, %esp
        popa
        add $8, %esp                        /* drop vector number and error code */
        iret

        .section .note.GNU-stack, "", @progbits
