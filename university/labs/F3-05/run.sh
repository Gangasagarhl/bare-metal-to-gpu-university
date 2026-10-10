#!/usr/bin/env bash
# Runs for F3-05: the uni-rv lock kernel on two emulated RISC-V CPUs in QEMU, with
# the lock-order bug (frozen) and with the fix (fixed); evidence for the forensic lab.
set -u
cc=riscv64-linux-gnu-g++
ccver="$($cc --version | head -n 1)"
qver="$(qemu-system-riscv64 --version | head -n 1)"
gxx="$(g++ --version | head -n 1)"
kflags="-march=rv64gc -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -fno-exceptions -fno-rtti -O2 -g -Wall -Wextra -Werror -std=c++20 -static -Wl,--build-id=none -Wl,--no-warn-rwx-segments"
qemu="qemu-system-riscv64 -machine virt -bios none -nographic -smp 2"
header() {   # header <name> <listing> <command> <toolchain>
    {
        echo "listing:   $2"
        echo "toolchain: $4"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container); emulated machine: QEMU virt, 2 RISC-V harts"
    } > "$1.log"
}
status=0
# 1. the kernels
$cc $kflags -T kernel.ld entry.S locks.cc -o frozen.elf || exit 1
$cc $kflags -DSAME_ORDER -T kernel.ld entry.S locks.cc -o fixed.elf || exit 1

# 2. the frozen machine: serial log; while it hangs, ask QEMU where both CPUs are
header frozen "entry.S + locks.cc (lock order bug)" "$qemu -kernel frozen.elf -qmp unix:qmp.sock,server,nowait  (timeout 6 s)" "$ccver; $qver"
rm -f qmp.sock
timeout 6 $qemu -kernel frozen.elf -qmp unix:qmp.sock,server,nowait > frozen.out 2>&1 < /dev/null &
qpid=$!
sleep 3
header cpus "QMP command 'info registers -a' sent to the frozen machine 3 s after start, then addr2line" \
    "python3 qmp_regs.py qmp.sock; riscv64-linux-gnu-addr2line -f -C -e frozen.elf <pc>" "$qver; $(python3 --version)"
python3 qmp_regs.py qmp.sock > cpus.raw 2>&1
wait $qpid; rc=$?
echo "exit code: $rc (124 = QEMU stopped by the 6 s time limit: the machine never powered off)" >> frozen.log
sed -i -E "s/from pid [0-9]+/from pid <timeout's pid>/" frozen.out
{
    cat cpus.raw
    echo "--- program counters mapped to source lines (addr2line) ---"
    for pc in $(awk '$1 == "pc" {print $2}' cpus.raw | sort -u); do
        echo "0x$pc:"; riscv64-linux-gnu-addr2line -f -C -e frozen.elf "0x$pc" | sed 's/^/    /'
    done
} > cpus.out
sed -i "s#$(pwd)/##g" cpus.out
echo "exit code: 0" >> cpus.log
rm -f cpus.raw qmp.sock

# 3. the same kernel with one lock order everywhere
header fixed "entry.S + locks.cc built with -DSAME_ORDER" "$qemu -kernel fixed.elf  (timeout 6 s)" "$ccver; $qver"
timeout 6 $qemu -kernel fixed.elf > fixed.out 2>&1 < /dev/null; rc=$?
echo "exit code: $rc (0 = the kernel powered the machine off through the test device)" >> fixed.log
[ "$rc" = 0 ] || status=1

# 4. a host tool reads the frozen trace and finds the wait-for cycle
hflags="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
header lockgraph "lockgraph.cc reading frozen.out" "g++ $hflags lockgraph.cc -o lockgraph && ./lockgraph < frozen.out" "$gxx"
g++ $hflags lockgraph.cc -o .bin_lockgraph || { echo "result:    BUILD FAILED" >> lockgraph.log; exit 1; }
./.bin_lockgraph < frozen.out > lockgraph.out 2>&1; echo "exit code: $?" >> lockgraph.log
echo "stdin:     frozen.out" >> lockgraph.log
# 5. the instruction the stuck CPUs were executing: the spin loop of acquire()
odver="$(riscv64-linux-gnu-objdump --version | head -n 1)"
header spin_loop "frozen.elf disassembled: the loop of acquire() around the atomic swap" \
    "riscv64-linux-gnu-objdump -d -C --no-show-raw-insn frozen.elf (function acquire, 4 lines before and 2 after amoswap)" "$odver"
riscv64-linux-gnu-objdump -d -C --no-show-raw-insn frozen.elf \
    | awk '/>:$/ { f = ($0 ~ /::acquire\(/) } f' | grep -B4 -A2 'amoswap' \
    | sed 's/(anonymous namespace)::acquire((anonymous namespace)::Spinlock&, long, long)/acquire/' > spin_loop.out
echo "exit code: $?" >> spin_loop.log
rm -f .bin_lockgraph frozen.elf fixed.elf
exit $status
