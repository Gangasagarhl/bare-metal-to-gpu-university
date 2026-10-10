/* boot.S - F4-28: RV64 S-mode entry. OpenSBI (QEMU's default firmware, M-mode) jumps here
 * in S-mode with a0 = this hart's ID and a1 = physical address of the devicetree blob,
 * after the SBI specification's boot convention (title only, pending verification;
 * the lab run prints both registers). Only one hart arrives here; the others stay
 * stopped in the firmware until the kernel starts them through SBI HSM (F4-29). */
        .section .text.entry, "ax"
        .globl _start
_start:
        csrw    sie, zero                   /* no interrupts until F4-29 sets them up */
        mv      s0, a0                      /* hart ID */
        mv      s1, a1                      /* devicetree */
.option push
.option norelax                             /* gp is not set up yet: no gp-relative relaxing */
        la      gp, __global_pointer$
.option pop
        la      sp, __stack_top             /* boot hart stack (linker.ld) */
        la      t0, trap_vector             /* stvec: direct mode (low two bits 00) */
        csrw    stvec, t0
        la      t0, __bss_start
        la      t1, __bss_end
1:      bgeu    t0, t1, 2f
        sd      zero, 0(t0)
        addi    t0, t0, 8
        j       1b
2:      call    run_global_constructors     /* F3-18 cxxrt.cc, unchanged */
        mv      a0, s0                      /* kmain(hartid, dtb) */
        mv      a1, s1
        call    kmain
3:      wfi
        j       3b
