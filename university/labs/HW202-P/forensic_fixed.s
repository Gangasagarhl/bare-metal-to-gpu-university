; clamp_fixed.s - the answer key's version: the address update moved up AND the store's offset
;                 changed from 0 to -1, because r1 now already points at the next reading
        .data 12 75 4 51 9 50
        addi r1, r0, 0
        addi r2, r0, 6
        addi r6, r0, 20
loop:   lw   r4, 0(r1)
        addi r1, r1, 1      ; fills the load-use bubble
        slt  r5, r4, r6
        beq  r5, r0, keep
        sw   r6, -1(r1)     ; the reading just loaded is now at r1 - 1
keep:   addi r2, r2, -1
        bne  r2, r0, loop
        addi r1, r0, 0
        addi r2, r0, 6
        addi r3, r0, 0
sum:    lw   r4, 0(r1)
        addi r1, r1, 1
        add  r3, r3, r4
        addi r2, r2, -1
        bne  r2, r0, sum
        out  r3
        halt
