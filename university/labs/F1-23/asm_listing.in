; sum.s - add the five numbers stored at data addresses 0..4 and output the sum
        .data 7 3 9 4 6
        addi r1, r0, 0      ; r1 = address of the next number
        addi r2, r0, 5      ; r2 = how many numbers are left
        addi r3, r0, 0      ; r3 = running sum
loop:   lw   r4, 0(r1)      ; r4 = memory[r1]
        add  r3, r3, r4     ; sum = sum + r4
        addi r1, r1, 1      ; move to the next address
        addi r2, r2, -1     ; one number fewer to go
        bne  r2, r0, loop   ; repeat while r2 is not 0
        out  r3             ; send the sum to the output port
        halt
