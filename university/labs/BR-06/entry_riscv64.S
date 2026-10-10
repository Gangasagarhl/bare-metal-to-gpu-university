/* entry_riscv64.S - BR-06: RISC-V side of the boot contract (QEMU virt).
 * Two ways in, same file: through OpenSBI (QEMU's default firmware) we arrive in S-mode at
 * 0x80200000; with -bios none QEMU's reset code jumps to 0x80000000 in M-mode. Both pass
 * a0 = hart ID and a1 = devicetree address (the lab run prints both). */
        .section .text.entry, "ax"
        .globl _start
_start:
.option push
.option norelax
        la      gp, __global_pointer$
.option pop
        la      sp, __stack_top
        call    arch_entry                  /* arch_entry(a0 = hart ID, a1 = devicetree) */
1:      wfi
        j       1b

/* try_read_misa(u64* out): reads misa, a machine-level CSR. Returns 1 if the read worked,
 * 0 if it trapped. Only meaningful while stvec points at probe_trap below. */
        .text
        .globl try_read_misa
try_read_misa:
        li      t1, 1                       /* assume it works */
        csrr    t0, misa                    /* illegal instruction below M-mode */
        sd      t0, 0(a0)
        mv      a0, t1
        ret

/* S-mode trap handler used only around try_read_misa: note the cause, skip the 4-byte
 * csrr, report failure through t0 = 0, t1 = 0. Direct mode: aligned to 4 bytes. */
        .balign 4
        .globl probe_trap
probe_trap:
        csrr    t2, scause
        la      t3, g_scause
        sd      t2, 0(t3)
        csrr    t2, sepc
        addi    t2, t2, 4
        csrw    sepc, t2
        li      t0, 0
        li      t1, 0
        sret

/* S-mode handler for everything after the probe: report and stop. */
        .balign 4
        .globl fatal_trap
fatal_trap:
        call    arch_fatal
2:      j       2b

        .section .note.GNU-stack, "", %progbits
