; range_sched.s - the answer key's scheduling of range.s (practical Part 3): no data stall with
;                 forwarding. Two moves: "addi r2, r0, 4" now separates "lw r6" from its reader
;                 "add r7"; "addi r1, r1, 1" now separates "lw r4" from its reader "slt", so the
;                 address update happens before the compares (they do not use r1). Same output.
        .data 14 3 27 9
        addi r1, r0, 0      ; r1 = address of the next reading
        lw   r6, 0(r1)      ; r6 = largest so far  (the first reading)
        addi r2, r0, 4      ; r2 = readings left   (moved: fills the load-use bubble)
        add  r7, r6, r0     ; r7 = smallest so far (the first reading)
loop:   lw   r4, 0(r1)      ; r4 = reading
        addi r1, r1, 1      ; next address         (moved: fills the load-use bubble)
        slt  r5, r6, r4     ; r5 = 1 if largest < reading
        beq  r5, r0, skip1  ; not larger: keep the old largest
        add  r6, r4, r0     ; new largest
skip1:  slt  r5, r4, r7     ; r5 = 1 if reading < smallest
        beq  r5, r0, skip2  ; not smaller: keep the old smallest
        add  r7, r4, r0     ; new smallest
skip2:  addi r2, r2, -1     ; one reading fewer
        bne  r2, r0, loop   ; repeat while readings are left
        sub  r3, r6, r7     ; r3 = largest - smallest
        out  r3             ; send the range to the output port
        sw   r3, 4(r0)      ; keep it at data address 4
        halt
