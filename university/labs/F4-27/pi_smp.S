/* pi_smp.S - F4-27: where a Pi core starts after the boot core releases it from the spin
 * table. A spin-table core gets no argument (unlike PSCI CPU_ON's context_id), so it finds
 * its own index in MPIDR_EL1.Aff0 and then continues in F4-26's secondary_entry, which
 * expects the index in x0. The spin-table protocol itself is in the chapter (unverified box). */
        .text
        .globl pi_secondary_entry
pi_secondary_entry:
        mrs     x0, mpidr_el1
        and     x0, x0, #0xff               /* Aff0 = core number 0..3 on the BCM2837 */
        b       secondary_entry
