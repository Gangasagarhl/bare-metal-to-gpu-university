/* isr.S - DR301 F4-03: 48 interrupt entry stubs (32-bit protected mode) and the common
 * entry code. Vectors 0-31 are CPU exceptions; 32-47 are the 16 PIC lines after remapping.
 * The CPU pushes an error code for vectors 8, 10-14 and 17 (Intel SDM Vol. 3,
 * "Interrupt and Exception Handling"; written from memory, pending verification); every
 * other stub pushes a dummy 0 so that all frames have the same layout. */
        .text
        .globl isr_stubs
        .balign 16
isr_stubs:
        .set vec, 0
        .rept 48
        .balign 16
        .if (vec == 8) || ((vec >= 10) && (vec <= 14)) || (vec == 17)
        .else
        pushl $0                            /* dummy error code */
        .endif
        pushl $vec                          /* vector number */
        jmp isr_common
        .set vec, vec + 1
        .endr

isr_common:
        pusha                               /* eax ecx edx ebx esp ebp esi edi */
        mov %esp, %eax
        push %eax                           /* argument: pointer to the frame */
        cld
        call interrupt_dispatch
        add $4, %esp
        popa
        add $8, %esp                        /* drop vector number and error code */
        iret

        .section .note.GNU-stack, "", @progbits
