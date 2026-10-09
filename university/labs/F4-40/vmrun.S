/* vmrun.S - svm_run(vmcb_pa, regs): enter the guest once, come back at its next #VMEXIT.
 * VMRUN saves the host's state in the host-save area (MSR VM_HSAVE_PA) and loads the
 * guest's from the VMCB; RAX and RSP are in the VMCB, the other general registers are
 * not: we load them from GuestRegs before VMRUN and store them back after #VMEXIT.
 * Interrupts: CLGI blocks them, STI sets the host's IF that the VMCB's V_INTR_MASKING
 * makes the guard for physical interrupts while the guest runs (an interrupt then causes
 * #VMEXIT with code SVM_EXIT_INTR); after #VMEXIT, STGI lets the host take it. */
    .code64
    .global svm_run
svm_run:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    push %rsi                       /* GuestRegs*, needed after the exit */
    mov %rdi, %rax                  /* RAX = physical address of the VMCB */
    mov 0x00(%rsi), %rbx
    mov 0x08(%rsi), %rcx
    mov 0x10(%rsi), %rdx
    mov 0x20(%rsi), %rdi
    mov 0x28(%rsi), %rbp
    mov 0x30(%rsi), %r8
    mov 0x38(%rsi), %r9
    mov 0x40(%rsi), %r10
    mov 0x48(%rsi), %r11
    mov 0x50(%rsi), %r12
    mov 0x58(%rsi), %r13
    mov 0x60(%rsi), %r14
    mov 0x68(%rsi), %r15
    mov 0x18(%rsi), %rsi
    clgi
    sti
    vmrun %rax
    stgi                            /* a pending host interrupt is taken here */
    nop
    cli
    push %rsi                       /* the guest's RSI */
    mov 8(%rsp), %rsi               /* GuestRegs* */
    popq 0x18(%rsi)
    mov %rbx, 0x00(%rsi)
    mov %rcx, 0x08(%rsi)
    mov %rdx, 0x10(%rsi)
    mov %rdi, 0x20(%rsi)
    mov %rbp, 0x28(%rsi)
    mov %r8,  0x30(%rsi)
    mov %r9,  0x38(%rsi)
    mov %r10, 0x40(%rsi)
    mov %r11, 0x48(%rsi)
    mov %r12, 0x50(%rsi)
    mov %r13, 0x58(%rsi)
    mov %r14, 0x60(%rsi)
    mov %r15, 0x68(%rsi)
    pop %rsi
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    ret
