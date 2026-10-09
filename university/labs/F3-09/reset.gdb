# reset.gdb - milestone P3: look at the CPU before the first instruction, then step.
# Used as: gdb -batch -nx -ex "target remote 127.0.0.1:<port>" -x reset.gdb
echo == registers at power-on, as QEMU's monitor shows them (CS has a hidden base) ==\n
monitor info registers
echo == the 16 bytes at the reset vector ==\n
x/16xb 0xfffffff0
echo == eight single steps: instruction pointer and code segment after each ==\n
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
stepi
info registers rip cs
echo == registers after eight steps ==\n
monitor info registers
kill
