/* crt0.S - user-space start-up: the kernel jumps here with RSP pointing at argc,
 * followed by argv[], NULL, envp[], NULL and the auxiliary vector (F3-30). */
    .text
    .global _start
_start:
    movq (%rsp), %rdi          /* argc */
    leaq 8(%rsp), %rsi         /* argv */
    call main
    movl %eax, %edi            /* exit(main's return value) */
    xorl %eax, %eax            /* system call 0: exit */
    syscall
1:  jmp 1b
    .section .note.GNU-stack, "", @progbits
