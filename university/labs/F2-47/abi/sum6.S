# sum6.S: long sum6(long a, long b, long c, long d, long e, long f), written by hand
# following the System V AMD64 calling convention (chapter F2-47, D2): the six integer
# arguments arrive in rdi, rsi, rdx, rcx, r8, r9; the result goes back in rax. It uses only
# caller-saved registers, so it needs no prologue.
    .text
    .globl sum6
    .type sum6, @function
sum6:
    mov %rdi, %rax
    add %rsi, %rax
    add %rdx, %rax
    add %rcx, %rax
    add %r8, %rax
    add %r9, %rax
    ret
    .size sum6, .-sum6
    .section .note.GNU-stack, "", @progbits
