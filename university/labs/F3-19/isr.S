/* isr.S - F3-19: 256 interrupt entry stubs and the common entry code.
 * Each stub is 16-byte aligned, so stub n starts at isr_stubs + 16 * n. Vectors for which
 * the CPU pushes an error code (8, 10-14, 17, 21, 29, 30; Intel SDM Vol. 3, "Interrupt and
 * Exception Handling", pending verification) push only the vector number; all others push a
 * dummy error code 0 first, so every handler sees the same frame layout. */
        .text
        .globl isr_stubs
        .balign 16
isr_stubs:
        .set vec, 0
        .rept 256
        .balign 16
        .if (vec == 8) || ((vec >= 10) && (vec <= 14)) || (vec == 17) || (vec == 21) || (vec == 29) || (vec == 30)
        .else
        pushq $0                            /* dummy error code */
        .endif
        pushq $vec                          /* vector number */
        jmp isr_common
        .set vec, vec + 1
        .endr

isr_common:
        push %rax                           /* save every general-purpose register: */
        push %rbx                           /* together they form struct InterruptFrame */
        push %rcx
        push %rdx
        push %rsi
        push %rdi
        push %rbp
        push %r8
        push %r9
        push %r10
        push %r11
        push %r12
        push %r13
        push %r14
        push %r15
        mov %rsp, %rdi                      /* argument: pointer to the frame */
        cld                                 /* the C++ code expects DF = 0 (System V ABI) */
        call interrupt_dispatch
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %r11
        pop %r10
        pop %r9
        pop %r8
        pop %rbp
        pop %rdi
        pop %rsi
        pop %rdx
        pop %rcx
        pop %rbx
        pop %rax
        add $16, %rsp                       /* drop vector number and error code */
        iretq

        .section .note.GNU-stack, "", @progbits
