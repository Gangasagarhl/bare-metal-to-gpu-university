/* rv_start.S - BR-07 forensic lab: entry of Lena's RISC-V kernel (S-mode, after OpenSBI).
 * OpenSBI jumps to the kernel at 0x80200000 with a0 = hart id and a1 = devicetree address
 * (the kernel ignores both: that is part of the evidence). */
        .section .text.start, "ax"
        .globl _start
_start:
        la      sp, stack_top
        call    kmain
1:      wfi
        j       1b

        .bss
        .balign 16
        .space  4096
stack_top:
