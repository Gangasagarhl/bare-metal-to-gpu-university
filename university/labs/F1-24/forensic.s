; count_below.s - Kofi's program: count how many of the four numbers are below 5 (should output 2)
        .data 3 8 1 6
        addi r1, r0, 0      ; r1 = address
        addi r2, r0, 4      ; r2 = numbers left
        addi r6, r0, 5      ; r6 = the limit, 5
loop:   lw   r4, 0(r1)      ; r4 = memory[r1]
        slt  r5, r4, r6     ; r5 = 1 if r4 < 5, else 0
        add  r0, r0, r5     ; count = count + r5
        addi r1, r1, 1
        addi r2, r2, -1
        bne  r2, r0, loop
        out  r0             ; output the count
        halt
