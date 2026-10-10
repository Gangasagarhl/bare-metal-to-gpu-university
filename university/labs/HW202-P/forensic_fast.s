; clamp_fast.s - Ravi's "faster" clamp: the address update moved up to fill the load-use bubble
        .data 12 75 4 51 9 50
        addi r1, r0, 0      ; r1 = address of the next reading
        addi r2, r0, 6      ; r2 = readings left
        addi r6, r0, 20     ; r6 = the floor, 20
loop:   lw   r4, 0(r1)      ; r4 = reading
        addi r1, r1, 1      ; moved here: fills the bubble after the load
        slt  r5, r4, r6     ; r5 = 1 if reading < 20
        beq  r5, r0, keep   ; not below the floor: keep it
        sw   r6, 0(r1)      ; replace the reading by the floor
keep:   addi r2, r2, -1     ; one reading fewer
        bne  r2, r0, loop
        addi r1, r0, 0      ; second loop: add up the six readings
        addi r2, r0, 6
        addi r3, r0, 0
sum:    lw   r4, 0(r1)
        addi r1, r1, 1
        add  r3, r3, r4
        addi r2, r2, -1
        bne  r2, r0, sum
        out  r3
        halt
