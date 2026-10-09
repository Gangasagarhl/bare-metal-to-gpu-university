#!/usr/bin/env bash
# F3-36 lab steps (milestone H1 in emulation): look at the emulated chip's memory map, build
# freestanding C++ firmware with our own start-up code and linker script, report its size,
# boot it, capture the LED writes, debug the reset handler, and build the forensic variant.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU="qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -semihosting-config enable=on,target=native"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
QV="$(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; run in QEMU's emulated mps2-an385 board, not on a real microcontroller"
build() {  # build <startup source> <elf name> [map file]
    clang++ $ARMFLAGS -c "$1" -o $B/startup.o &&
    clang++ $ARMFLAGS -c main.cc -o $B/main.o &&
    ld.lld -T os305.ld --gc-sections ${3:+-Map $3} $B/startup.o $B/main.o -o "$B/$2"
}

# 1. the emulated chip's memory map (where flash, RAM and peripherals are)
(echo "info mtree -f"; sleep 1; echo quit) | timeout 10 qemu-system-arm -M mps2-an385 -nographic -monitor stdio -serial null -S 2>&1 \
    | tr -d '\r' | sed 's/\x1b\[[0-9;]*[A-Za-z]//g' \
    | awk '/Root memory region: system/{p=1; next} p && /^$/{exit} p' \
    | grep -E 'ssram1|ssram23|uart|fpgaio|cmsdk-apb-timer' | head -n 12 > mtree.out
rc=$?
rec mtree "(QEMU monitor command)" "$QV" "qemu-system-arm -M mps2-an385 -nographic -monitor stdio -S, then: info mtree -f (lines for memory, UART, timers, FPGA I/O kept)" "$rc"
[ -s mtree.out ] || status=1

# 2. build
cmd="clang++ $ARMFLAGS -c startup.cc main.cc && ld.lld -T os305.ld --gc-sections -Map firmware.map startup.o main.o -o firmware.elf"
{ build startup.cc firmware.elf $B/firmware.map && echo "build ok: firmware.elf"; } > build.out 2>&1; rc=$?
rec build "startup.cc main.cc board.h os305.ld" "$TC" "$cmd" "$rc" "note:      target thumbv7m-none-eabi (Armv7-M, Cortex-M3), freestanding, no vendor framework"
[ "$rc" = 0 ] || status=1

# 3. size and symbol report (H1: "the image size and RAM use are reported and fit the chip")
{ llvm-size -A $B/firmware.elf | grep -E '^(section|\.text|\.data|\.bss)'; } | sed "s#$B/##" > size.out 2>&1; rc=$?
rec size firmware.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A firmware.elf (allocated sections only)" "$rc"
llvm-nm -n $B/firmware.elf | grep -v ' W ' > symbols.out 2>&1; rc=$?
rec symbols firmware.elf "$(llvm-nm --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-nm -n firmware.elf | grep -v ' W ' (weak handler aliases omitted)" "$rc"

# 4. boot: UART output; the firmware ends the emulation through semihosting
timeout 20 $QEMU -kernel $B/firmware.elf > boot.out 2>&1; rc=$?
rec boot firmware.elf "$QV" "timeout 20 $QEMU -kernel firmware.elf" "$rc (0 = the firmware reported success through semihosting SYS_EXIT)" "$HW"
[ "$rc" = 0 ] || status=1
grep -q "self-check PASS" boot.out || status=1

# 5. what the LED register saw: QEMU's trace of every write to the FPGA I/O block
timeout 20 $QEMU -trace mps2_fpgaio_write -D $B/trace.txt -kernel $B/firmware.elf > /dev/null 2>&1; rc=$?
sed -E 's/^[0-9]+@[0-9.]+://' $B/trace.txt | grep mps2_fpgaio_write > trace.out
rec trace firmware.elf "$QV" "timeout 20 $QEMU -trace mps2_fpgaio_write -D trace.txt -kernel firmware.elf" "$rc" "$HW"

# 6. debugger session: breakpoint in the reset handler (H1 acceptance), through QEMU's GDB stub
sock="$B/gdb.sock"
qemu-system-arm -M mps2-an385 -nographic -monitor none -serial file:$B/uart.txt -semihosting-config enable=on,target=native \
    -kernel $B/firmware.elf -S -chardev socket,path=$sock,server=on,wait=off,id=g0 -gdb chardev:g0 > $B/qemu.txt 2>&1 &
qpid=$!
sleep 1
timeout 60 lldb-18 --batch -o "process connect --plugin gdb-remote unix-connect://$sock" -s session.lldb -o kill \
    $B/firmware.elf > session.out 2>&1; rc=$?
kill $qpid 2>/dev/null; wait $qpid 2>/dev/null
sed -i -E -e "s#$(pwd)/##g" -e "s#$B/##g" -e "s#unix-connect://[^ ]*#unix-connect://[socket path removed]#" -e '/^\(lldb\) kill$/,$d' session.out
rec session "session.lldb on firmware.elf" "$(lldb-18 --version | head -n 1); $QV" \
    "qemu-system-arm -M mps2-an385 -S -gdb (unix socket) -kernel firmware.elf; lldb-18 --batch -o 'process connect --plugin gdb-remote ...' -s session.lldb firmware.elf" \
    "$rc" "hardware:  untested on hardware; QEMU's GDB stub stood in for a debug probe; LLDB stood in for GDB (the container's GDB is x86-only)"
grep -q "stop reason = breakpoint 1.1" session.out || status=1

# 7. forensic: the same firmware with broken_startup.cc
{ build broken_startup.cc broken.elf && timeout 20 $QEMU -kernel $B/broken.elf; } > forensic_boot.out 2>&1; rc=$?
rec forensic_boot "broken_startup.cc main.cc" "$TC; $QV" "same build as step 2 with broken_startup.cc; timeout 20 $QEMU -kernel broken.elf" \
    "$rc (1 = the firmware reported failure through semihosting; expected for this forensic variant)" "$HW"
[ "$rc" = 1 ] || status=1
timeout 20 $QEMU -trace mps2_fpgaio_write -D $B/ftrace.txt -kernel $B/broken.elf > /dev/null 2>&1; rc=$?
sed -E 's/^[0-9]+@[0-9.]+://' $B/ftrace.txt | grep mps2_fpgaio_write > forensic_trace.out
rec forensic_trace broken.elf "$QV" "timeout 20 $QEMU -trace mps2_fpgaio_write -D trace.txt -kernel broken.elf" "$rc (1 = failure reported through semihosting; expected for this forensic variant)" "$HW"
{
    echo "--- sections of broken.elf (llvm-objdump -h) ---"
    llvm-objdump -h $B/broken.elf | grep -E 'Idx|\.text|\.data|\.bss'
    echo "--- symbols around the end of .text (llvm-nm -n) ---"
    llvm-nm -n $B/broken.elf | grep -E 'GLOBAL__sub|__init_array|__text_end|__data_load'
    echo "--- Reset_Handler of broken.elf (llvm-objdump -d) ---"
    llvm-objdump -d --no-show-raw-insn $B/broken.elf | sed -n '/<Reset_Handler>:/,/^$/p'
} | sed "s#$B/##g" > forensic_elf.out 2>&1
rec forensic_elf broken.elf "$(llvm-objdump --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-objdump -h / llvm-nm -n / llvm-objdump -d on broken.elf" 0

sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' boot.out forensic_boot.out
sed -e "s#$B/##g" -e '/\.debug_/d' $B/firmware.map | head -n 60 > firmware.map.txt
rm -rf "$B"
exit $status
