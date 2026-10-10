#!/usr/bin/env bash
# F1-77 lab steps: build the SysTick program, then debug it through QEMU's GDB stub, which
# plays the part of the debug probe. The container's gdb is built for x86 only, so the
# debugger used is LLDB 18, which speaks the same GDB remote serial protocol.
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
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -O0 -g -Wall -Wextra -Wpedantic -Werror"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
DBG="$(lldb-18 --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU's GDB stub stood in for a JTAG/SWD probe (no probe, no board)"

build() {  # build <app source> <elf name>
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c "$1" -o $B/app.o &&
    ld.lld -T mps2_an385.ld --gc-sections $B/startup.o $B/app.o -o "$B/$2"
}
debug() {  # debug <elf> <command file> <out file>: start QEMU halted, attach LLDB, run commands
    local sock="$B/gdb.sock"
    rm -f "$sock"
    qemu-system-arm -M mps2-an385 -nographic -monitor none -serial file:$B/uart.txt -kernel "$B/$1" -S \
        -chardev socket,path=$sock,server=on,wait=off,id=g0 -gdb chardev:g0 > $B/qemu.txt 2>&1 &
    local qpid=$!
    sleep 1
    timeout 60 lldb-18 --batch -o "process connect --plugin gdb-remote unix-connect://$sock" \
        -s "$2" -o 'kill' "$B/$1" > "$3" 2>&1
    local rc=$?
    kill $qpid 2>/dev/null; wait $qpid 2>/dev/null
    sed -i -E -e "s#$(pwd)/##g" -e "s#$B/##g" -e "s#unix-connect://[^ ]*#unix-connect://[socket path removed]#" \
        -e '/^\(lldb\) kill$/,$d' "$3"
    return $rc
}

# 1. build and run normally (UART output)
{
    build app.cc app.elf &&
    timeout 3 qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -kernel $B/app.elf
} > app_run.out 2>&1; rc=$?
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' app_run.out
rec app_run "startup.cc app.cc mps2_an385.ld" "$TC" \
    "clang++ $ARMFLAGS -c startup.cc app.cc && ld.lld -T mps2_an385.ld --gc-sections startup.o app.o -o app.elf && timeout 3 qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -kernel app.elf" \
    "$rc (124 = stopped by the 3 s time limit; the firmware loops forever by design)" "$HW"
grep -q "3 ticks seen" app_run.out || status=1

# 2. the debug session (H1 acceptance: a breakpoint in the reset handler is hit)
debug app.elf session.lldb session.out; rc=$?
rec session "session.lldb on app.elf" "$DBG" \
    "qemu-system-arm -M mps2-an385 -S -gdb (unix socket) -kernel app.elf; lldb-18 --batch -o 'process connect --plugin gdb-remote ...' -s session.lldb app.elf" \
    "$rc" "$HW"
grep -q "stop reason = breakpoint 1.1" session.out || status=1

# 3. forensic: Joon's app_v2.cc
{
    build app_v2.cc app_v2.elf &&
    llvm-nm -n $B/app_v2.elf | grep -i -E 'handler|vector_table|main'
} > forensic_nm.out 2>&1; rc=$?
sed -i "s#$B/##g" forensic_nm.out
rec forensic_nm "startup.cc app_v2.cc" "$TC; $(llvm-nm --version | sed -n 2p | sed 's/^ *//')" \
    "build as step 1 with app_v2.cc; llvm-nm -n app_v2.elf | grep -i -E 'handler|vector_table|main'" "$rc" \
    "note:      the build succeeds with no warning"
[ "$rc" = 0 ] || status=1
debug app_v2.elf forensic.lldb forensic_session.out; rc=$?
cp $B/uart.txt forensic_uart.out 2>/dev/null || : > forensic_uart.out
rec forensic_session "forensic.lldb on app_v2.elf" "$DBG" \
    "qemu-system-arm -M mps2-an385 -S -gdb (unix socket) -kernel app_v2.elf; lldb-18 --batch -s forensic.lldb app_v2.elf" "$rc" "$HW"
rec forensic_uart "UART output during the forensic session" "$(qemu-system-arm --version | head -n 1)" \
    "-serial file:uart.txt (same QEMU process as forensic_session)" 0 "$HW"
rm -rf "$B"
exit $status
