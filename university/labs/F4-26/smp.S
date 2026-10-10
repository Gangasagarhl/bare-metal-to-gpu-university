/* smp.S - F4-26: where a secondary CPU starts after PSCI CPU_ON. The firmware starts it at
 * secondary_entry with the MMU off and x0 = the context_id the boot CPU passed (here: the
 * CPU's index), at the boot CPU's exception level (PSCI specification, CPU_ON; title only,
 * pending verification; the lab run prints what each CPU sees). */
        .text
        .globl secondary_entry
secondary_entry:
        mov     x19, x0                     /* CPU index */
        mrs     x20, CurrentEL
        lsr     x20, x20, #2
        and     x20, x20, #3
        cmp     x20, #2
        b.ne    1f
        mov     x0, #(1 << 31)              /* same EL2 -> EL1 drop as boot.S */
        msr     hcr_el2, x0
        mov     x0, #3
        msr     cnthctl_el2, x0
        msr     cntvoff_el2, xzr
        mov     x0, #0x3c5
        msr     spsr_el2, x0
        adr     x0, 1f
        msr     elr_el2, x0
        eret
1:      msr     daifset, #0xf
        ldr     x0, =__stack_top            /* stack of CPU n: __stack_top - n * 16 KiB */
        mov     x1, #0x4000
        msub    x0, x1, x19, x0
        mov     sp, x0
        ldr     x0, =vector_table
        msr     vbar_el1, x0
        isb
        mov     x0, x19
        mov     x1, x20
        bl      secondary_main              /* secondary_main(index, entry_el): never returns */
2:      wfe
        b       2b
