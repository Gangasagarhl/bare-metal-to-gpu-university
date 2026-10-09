// stage_start.S - F4-32: common entry of the firmware stages (ROM, BL2, BL31).
// Sets the stack, clears .bss, calls stage_main(x0, x1, x2, x3) with the registers the previous
// stage passed. Each stage is linked at its own fixed address by stage.ld.
    .section .text.head, "ax"
    .globl _start
_start:
    mrs     x9, mpidr_el1          // only CPU 0 boots; on a real SoC the boot ROM parks the others
    and     x9, x9, #0xff
    cbnz    x9, 9f
    ldr     x9, =__stack_top
    mov     sp, x9
    ldr     x9, =__bss_start
    ldr     x10, =__bss_end
1:  cmp     x9, x10
    b.hs    2f
    str     xzr, [x9], #8
    b       1b
2:  bl      stage_main
9:  wfe
    b       9b
