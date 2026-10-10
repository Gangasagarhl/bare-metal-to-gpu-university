# scale_fixed.S: the same function, now respecting the System V AMD64 convention:
# rbx is callee-saved, so a function that uses it must save and restore it. (Simpler still:
# use a caller-saved register such as rcx and save nothing.)
    .text
    .globl scale
    .type scale, @function
scale:
    push %rbx
    mov %rdi, %rbx
    lea (%rbx,%rbx,2), %rax
    add $1, %rax
    pop %rbx
    ret
    .size scale, .-scale
    .section .note.GNU-stack, "", @progbits
