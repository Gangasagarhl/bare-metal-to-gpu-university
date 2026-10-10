; sum_reordered.s - the same sum as sum.s, with the add moved away from the load
        .data 7 3 9 4 6
        addi r1, r0, 0      ; r1 = address of the next number
        addi r2, r0, 5      ; r2 = how many numbers are left
        addi r3, r0, 0      ; r3 = running sum
loop:   lw   r4, 0(r1)      ; r4 = memory[r1]
        addi r1, r1, 1      ; moved up: does not need r4
        addi r2, r2, -1     ; moved up: does not need r4
        add  r3, r3, r4     ; r4 is ready by now
        bne  r2, r0, loop
        out  r3
        halt
