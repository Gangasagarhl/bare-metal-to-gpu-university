# entry.S - first instructions of the uni-rv teaching kernel.
# QEMU's virt machine (-bios none) starts every hart at 0x80000000.
        .section .text.entry
        .globl _start
_start:
        csrr    t0, mhartid          # which hart (CPU) am I?
        la      sp, stacks           # bottom of the stack area
        li      t1, 4096             # 4 KiB stack per hart
        addi    t0, t0, 1
        mul     t1, t1, t0
        add     sp, sp, t1           # sp = stacks + (hartid + 1) * 4096
        csrr    a0, mhartid          # argument: hart id
        call    kmain
1:      wfi                          # kmain should not return; sleep if it does
        j       1b
