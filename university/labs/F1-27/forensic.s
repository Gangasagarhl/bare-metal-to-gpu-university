; count_big.s - count how many of the eight readings are larger than 50 (expected output: 3)
        .data 12 75 40 51 9 50 88 23
        addi r1, r0, 0      ; r1 = address of the next reading
        addi r2, r0, 8      ; r2 = readings left
        addi r3, r0, 0      ; r3 = count
        addi r6, r0, 31     ; r6 = 50, built in two steps
        addi r6, r6, 19     ;   (an immediate holds at most 31)
loop:   lw   r4, 0(r1)      ; r4 = reading
        slt  r5, r6, r4     ; r5 = 1 if 50 < reading
        add  r3, r3, r5     ; count = count + r5
        addi r1, r1, 1
        addi r2, r2, -1
        bne  r2, r0, loop
        out  r3
        halt
