; count_big_fixed.s - the answer key's version: one independent instruction after the load
        .data 12 75 40 51 9 50 88 23
        addi r1, r0, 0
        addi r2, r0, 8
        addi r3, r0, 0
        addi r6, r0, 31
        addi r6, r6, 19
loop:   lw   r4, 0(r1)
        addi r1, r1, 1      ; moved between the load and its first use
        slt  r5, r6, r4
        add  r3, r3, r5
        addi r2, r2, -1
        bne  r2, r0, loop
        out  r3
        halt
