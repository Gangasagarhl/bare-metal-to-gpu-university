/* syscall.S - the SYSCALL entry point and the first entry into user mode.
 * On SYSCALL the CPU saves the user RIP in RCX and RFLAGS in R11, loads CS/SS from
 * the STAR MSR, masks RFLAGS with the FMASK MSR and jumps to the address in LSTAR.
 * It does NOT switch stacks: that is our first job (AMD64 APM Vol. 2 "SYSCALL and
 * SYSRET"; Intel SDM "Fast System Calls in 64-Bit Mode"; titles only). */
    .text
    .global syscall_entry
syscall_entry:
    swapgs                     /* GS base: user value -> this CPU's Cpu object */
    movq %rsp, %gs:16          /* Cpu::user_rsp: park the user stack pointer */
    movq %gs:8, %rsp           /* Cpu::kernel_rsp: this thread's kernel stack */
    pushq %gs:16               /* SyscallFrame (syscall.cc), from the top down */
    pushq %r11                 /* user RFLAGS */
    pushq %rcx                 /* user RIP */
    pushq %rax                 /* system-call number */
    pushq %rdi                 /* arguments 1-6 */
    pushq %rsi
    pushq %rdx
    pushq %r10                 /* (the 4th argument travels in r10: SYSCALL overwrites rcx) */
    pushq %r8
    pushq %r9
    movq %rsp, %rdi
    sti                        /* system calls may be preempted */
    call syscall_dispatch      /* result in rax */
    cli
    popq %r9
    popq %r8
    popq %r10
    popq %rdx
    popq %rsi
    popq %rdi
    addq $8, %rsp              /* the number */
    popq %rcx
    popq %r11
    popq %rsp                  /* back on the user stack */
    swapgs
    sysretq                    /* RIP = RCX, RFLAGS = R11, CS/SS from STAR, ring 3 */

/* [[noreturn]] void enter_user(uint64_t rip, uint64_t rsp): first entry to ring 3.
 * Builds the frame an interrupt from ring 3 would have pushed and "returns" to it. */
    .global enter_user
enter_user:
    cli
    pushq $0x1b                /* SS  = user data selector 0x18 | RPL 3 */
    pushq %rsi                 /* RSP */
    pushq $0x202               /* RFLAGS: IF = 1 */
    pushq $0x23                /* CS  = user code selector 0x20 | RPL 3 */
    pushq %rdi                 /* RIP */
    xorl %eax, %eax            /* leave no kernel values in registers */
    xorl %ebx, %ebx
    xorl %ecx, %ecx
    xorl %edx, %edx
    xorl %esi, %esi
    xorl %edi, %edi
    xorl %ebp, %ebp
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    xorl %r10d, %r10d
    xorl %r11d, %r11d
    xorl %r12d, %r12d
    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d
    swapgs                     /* GS base back to the user value */
    iretq
