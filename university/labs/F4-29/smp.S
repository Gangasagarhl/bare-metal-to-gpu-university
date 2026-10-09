/* smp.S - F4-29: where a hart starts after SBI HSM hart_start. The firmware starts it in
 * S-mode at secondary_entry with a0 = its hart ID and a1 = the opaque value the boot hart
 * passed (here: a stack index), translation off (SBI specification, HSM extension; title
 * only, pending verification; the lab run prints what each hart sees). */
        .text
        .globl secondary_entry
secondary_entry:
        csrw    sie, zero
.option push
.option norelax
        la      gp, __global_pointer$
.option pop
        la      sp, __stack_top             /* stack n: __stack_top - n * 16 KiB */
        li      t0, 0x4000
        mul     t0, t0, a1
        sub     sp, sp, t0
        la      t0, trap_vector
        csrw    stvec, t0
        call    secondary_main              /* secondary_main(hartid, index): never returns */
1:      wfi
        j       1b
