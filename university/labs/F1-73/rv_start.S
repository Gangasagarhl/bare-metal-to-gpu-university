# rv_start.S - start-up code for the RISC-V comparison (QEMU virt machine, 64-bit).
# A RISC-V core does not read an initial stack pointer from a table at reset: it starts
# fetching instructions at a reset address chosen by the implementation, so the first
# instructions must set up the stack themselves.
    .section .text.start
    .global _start
_start:
    la    sp, __stack_top        # 1. stack pointer = end of RAM area (from the linker script)
    la    t0, __bss_start        # 2. zero .bss
    la    t1, __bss_end
1:  bgeu  t0, t1, 2f
    sd    zero, 0(t0)
    addi  t0, t0, 8
    j     1b
2:  call  main                   # 3. run the program
3:  wfi                          # 4. never return
    j     3b
