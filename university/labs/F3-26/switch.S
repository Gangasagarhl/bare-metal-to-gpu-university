/* switch.S - the context switch of the OS303 kernel.
 * void switch_context(uint64_t* save_rsp, uint64_t load_rsp)
 * Saves the callee-saved registers of the System V AMD64 ABI (rbx, rbp, r12-r15) on
 * the old thread's stack, stores its stack pointer, loads the new thread's stack
 * pointer and pops the same registers in reverse order. The 'ret' then returns into
 * the new thread at the point where it last called switch_context (or into
 * thread_start for a thread that never ran). Caller-saved registers need no saving:
 * the compiler already assumes a call destroys them. */
    .text
    .global switch_context
switch_context:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, (%rdi)          /* old->rsp = rsp */
    movq %rsi, %rsp            /* rsp = new->rsp: from here on we are the new thread */
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

/* First code a new thread runs. thread_create() put the entry function in r12 and its
 * argument in r13 (they are restored by the pops above). */
    .global thread_start
thread_start:
    call finish_switch         /* release the scheduler lock taken by the previous thread */
    sti                        /* new threads start with interrupts enabled */
    movq %r13, %rdi
    call *%r12
    call thread_exit_from_start
1:  hlt
    jmp 1b

/* First code of CPU 0's idle thread (its stack was prepared by thread_alloc). */
    .global idle_entry
idle_entry:
    call idle_loop
