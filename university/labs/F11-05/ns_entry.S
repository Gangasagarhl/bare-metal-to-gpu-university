// ns_entry.S - F11-05: entry and exception vectors of the normal-world program (non-secure EL1).
    .section .text.boot, "ax"
    .global _start
_start:
    ldr     x0, =ns_vectors
    msr     vbar_el1, x0
    isb
    bl      ns_main
1:  wfe
    b       1b

    .balign 0x800
ns_vectors:
    .rept 4                             // current EL with SP_EL0: unused
    .balign 0x80
    b       ns_unexpected
    .endr
    .balign 0x80                        // 0x200: synchronous exception, current EL with SP_ELx
    b       ns_sync_entry
    .rept 11
    .balign 0x80
    b       ns_unexpected
    .endr

ns_sync_entry:
    mrs     x0, esr_el1
    mrs     x1, far_el1
    mrs     x2, elr_el1
    bl      ns_fault                    // C++: reports and ends the run (does not return)
ns_unexpected:
    mov     x0, #0
    mov     x1, #0
    mrs     x2, elr_el1
    bl      ns_fault

// smc(function id, argument) -> x0 (SMC Calling Convention: id in w0/x0, arguments from x1)
    .global smc_call
smc_call:
    smc     #0
    ret
