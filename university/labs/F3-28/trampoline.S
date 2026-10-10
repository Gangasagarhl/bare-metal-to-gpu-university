/* trampoline.S - first instructions of an application processor (AP).
 * After INIT-SIPI-SIPI an AP starts in 16-bit real mode at the 4 KiB page named by the
 * SIPI vector (Intel SDM Vol. 3, "Multiple-Processor Management", MP initialization;
 * title only). smp.cc copies these bytes to physical address 0x8000 (vector 0x08) and
 * fills in the four variables at the end. The code climbs real mode -> protected mode
 * -> long mode with the boot CPU's page tables, then calls ap_entry(cpu) on its own
 * stack. Addresses are written as 0x8000 + (label - tramp_start) because the code runs
 * at 0x8000, not where the linker put it. */
    .set BASE, 0x8000
    .section .rodata.trampoline, "a"
    .global tramp_start, tramp_end, tramp_cr3, tramp_stack, tramp_entry, tramp_cpu
    .code16
tramp_start:
    cli
    cld
    xorw %ax, %ax
    movw %ax, %ds
    lgdtl BASE + (tramp_gdt_ptr - tramp_start)
    movl %cr0, %eax
    orl $1, %eax                         /* CR0.PE: protected mode */
    movl %eax, %cr0
    ljmpl $0x08, $(BASE + (tramp_pm - tramp_start))

    .code32
tramp_pm:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl %cr4, %eax
    orl $0x20, %eax                      /* CR4.PAE */
    movl %eax, %cr4
    movl BASE + (tramp_cr3 - tramp_start), %eax
    movl %eax, %cr3                      /* the boot CPU's PML4 */
    movl $0xC0000080, %ecx               /* IA32_EFER */
    rdmsr
    orl $0x100, %eax                     /* EFER.LME */
    wrmsr
    movl %cr0, %eax
    orl $0x80000000, %eax                /* CR0.PG: long mode becomes active */
    movl %eax, %cr0
    ljmp $0x18, $(BASE + (tramp_lm - tramp_start))

    .code64
tramp_lm:
    movq BASE + (tramp_stack - tramp_start), %rsp
    movq BASE + (tramp_cpu - tramp_start), %rdi
    movq BASE + (tramp_entry - tramp_start), %rax
    callq *%rax                          /* ap_entry(Cpu*) never returns */
1:  hlt
    jmp 1b

    .align 8
tramp_gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF             /* 0x08: 32-bit code */
    .quad 0x00CF92000000FFFF             /* 0x10: data */
    .quad 0x00AF9A000000FFFF             /* 0x18: 64-bit code */
tramp_gdt_ptr:
    .word 4 * 8 - 1
    .long BASE + (tramp_gdt - tramp_start)
    .align 8
tramp_cr3:   .quad 0
tramp_stack: .quad 0
tramp_entry: .quad 0
tramp_cpu:   .quad 0
tramp_end:
