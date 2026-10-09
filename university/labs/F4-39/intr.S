/* intr.S - interrupt entry stubs: vectors 0-31 (exceptions) and 32 (timer, IRQ 0).
 * Each stub pushes a dummy error code when the CPU does not push one, then the vector
 * number, saves the general registers and calls isr_dispatch(Frame*). */
    .code64
    .macro STUB num, has_err
    .global isr\num
isr\num:
    .if \has_err == 0
    pushq $0
    .endif
    pushq $\num
    jmp isr_common
    .endm

    STUB 0,0;  STUB 1,0;  STUB 2,0;  STUB 3,0;  STUB 4,0;  STUB 5,0;  STUB 6,0;  STUB 7,0
    STUB 8,1;  STUB 9,0;  STUB 10,1; STUB 11,1; STUB 12,1; STUB 13,1; STUB 14,1; STUB 15,0
    STUB 16,0; STUB 17,1; STUB 18,0; STUB 19,0; STUB 20,0; STUB 21,1; STUB 22,0; STUB 23,0
    STUB 24,0; STUB 25,0; STUB 26,0; STUB 27,0; STUB 28,0; STUB 29,1; STUB 30,1; STUB 31,0
    STUB 32,0

isr_common:
    push %rax; push %rbx; push %rcx; push %rdx; push %rsi; push %rdi; push %rbp
    push %r8;  push %r9;  push %r10; push %r11; push %r12; push %r13; push %r14; push %r15
    mov %rsp, %rdi
    cld
    call isr_dispatch
    pop %r15; pop %r14; pop %r13; pop %r12; pop %r11; pop %r10; pop %r9; pop %r8
    pop %rbp; pop %rdi; pop %rsi; pop %rdx; pop %rcx; pop %rbx; pop %rax
    add $16, %rsp                      /* vector number and error code */
    iretq

    .section .rodata
    .global isr_table
isr_table:
    .irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32
    .quad isr\n
    .endr
