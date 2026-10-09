/* trap.S - one 16-byte entry stub per interrupt vector, and the common entry/exit path.
 * For exceptions that push an error code (8, 10-14, 17, 21, 29, 30 in the Intel SDM
 * Vol. 3 exception table; title only) the CPU pushes it; for all others the stub
 * pushes 0 so that every TrapFrame has the same layout (cpu.h). */
    .text
    .align 16
    .global trap_stub_base
trap_stub_base:
    .set i, 0
    .rept 256
    .align 16
    .if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17) || (i == 21) || (i == 29) || (i == 30)
    .else
    pushq $0
    .endif
    pushq $i
    jmp trap_common
    .set i, i + 1
    .endr

trap_common:
    testb $3, 24(%rsp)          /* CS of the interrupted code: ring 3? */
    jz 1f
    swapgs                      /* coming from user mode: GS base -> this CPU's Cpu */
1:  pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %rdi             /* TrapFrame* */
    cld
    call trap_dispatch
    .global trap_return
trap_return:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16, %rsp              /* vector and error code */
    testb $3, 8(%rsp)           /* returning to ring 3? */
    jz 2f
    swapgs
2:  iretq

    .section .rodata
    .align 8
    .global trap_stubs
trap_stubs:
    .set i, 0
    .rept 256
    .quad trap_stub_base + 16 * i
    .set i, i + 1
    .endr
