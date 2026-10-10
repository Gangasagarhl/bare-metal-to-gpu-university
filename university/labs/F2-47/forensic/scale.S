# scale.S: long scale(long x) -- returns x * 3 + 1.
# Written by a teammate "for speed". Look at which registers it uses.
    .text
    .globl scale
    .type scale, @function
scale:
    mov %rdi, %rbx
    lea (%rbx,%rbx,2), %rax
    add $1, %rax
    ret
    .size scale, .-scale
    .section .note.GNU-stack, "", @progbits
