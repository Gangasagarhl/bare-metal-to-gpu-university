#!/usr/bin/env bash
# F1-74 lab: the memory map of the emulated STM32F100 (QEMU stm32vldiscovery):
# QEMU's memory tree, a firmware build with a map file, a boot that prints where each
# kind of object lives, and the forensic build whose vector table went missing.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
CXX=clang++
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMU_ARM="qemu-system-arm"
MACH="stm32vldiscovery"
CLANGV="$($CXX --version | head -n 1); $(ld.lld --version | head -n 1)"
QEMUV="$($QEMU_ARM --version | head -n 1)"
LLDBV="$(lldb-18 --version | head -n 1)"
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
EMU="hardware:  untested on hardware; run in QEMU's emulated STM32F100 board, not on a real chip"
clean() { sed -i -E -e "s#$B/##g" -e 's/from pid [0-9]+/from pid [pid removed]/' -e '/^\.\/run\.sh: line [0-9]+: +[0-9]+ Aborted/d' "$@"; }

# 1. the emulated chip's memory map, as QEMU itself reports it
(echo "info mtree -f"; sleep 1; echo quit) | timeout 10 $QEMU_ARM -M $MACH -nographic -monitor stdio -serial null -S 2>&1 \
    | tr -d '\r' | sed 's/\x1b\[[0-9;]*[A-Za-z]//g' \
    | awk '/Root memory region: system/{p=1; next} p && /^$/{exit} p' > mtree.out
rec mtree "(QEMU monitor command)" "$QEMUV" "qemu-system-arm -M $MACH -nographic -monitor stdio -S, then: info mtree -f" 0 \
    "note:      system address space only; 'prio -1000' regions are QEMU placeholders for devices it does not model"
[ -s mtree.out ] || status=1

# 2. build with a map file
cmd="$CXX $ARMFLAGS -c startup.cc main.cc && ld.lld -T stm32f100.ld --gc-sections -Map firmware.map startup.o main.o -o firmware.elf"
{
    $CXX $ARMFLAGS -c startup.cc -o $B/startup.o &&
    $CXX $ARMFLAGS -c main.cc -o $B/main.o &&
    ld.lld -T stm32f100.ld --gc-sections -Map $B/firmware.map $B/startup.o $B/main.o -o $B/firmware.elf &&
    llvm-size -B $B/firmware.elf
} > build.out 2>&1; rc=$?
rec build "startup.cc main.cc stm32f100.ld" "$CLANGV" "$cmd; llvm-size -B firmware.elf" "$rc"
[ "$rc" = 0 ] || status=1
sed -e '/ \.debug_loclists$/,$d' $B/firmware.map > map.out
rec map firmware.map "$CLANGV" "ld.lld ... -Map firmware.map (debug sections cut from the end)" 0

# 3. boot and print the tour
timeout 3 $QEMU_ARM -M $MACH -nographic -monitor none -serial stdio -kernel $B/firmware.elf > boot.out 2>&1; rc=$?
rec boot firmware.elf "$QEMUV" "timeout 3 qemu-system-arm -M $MACH -nographic -monitor none -serial stdio -kernel firmware.elf" \
    "$rc (124 = stopped by the 3 s time limit; the firmware loops forever by design)" "$EMU"
grep -q "^done" boot.out || status=1

# 4. forensic: the same program with startup_v2.cc
cmd="$CXX $ARMFLAGS -c startup_v2.cc && ld.lld -T stm32f100.ld --gc-sections --print-gc-sections -Map firmware_v2.map startup_v2.o main.o -o firmware_v2.elf"
{
    $CXX $ARMFLAGS -c startup_v2.cc -o $B/startup_v2.o &&
    ld.lld -T stm32f100.ld --gc-sections --print-gc-sections -Map $B/firmware_v2.map $B/startup_v2.o $B/main.o -o $B/firmware_v2.elf &&
    llvm-size -B $B/firmware_v2.elf
} > forensic_build.out 2>&1; rc=$?
rec forensic_build "startup_v2.cc main.cc stm32f100.ld" "$CLANGV" "$cmd; llvm-size -B firmware_v2.elf" "$rc" \
    "note:      the build succeeds: no error and no warning"
sed -e '/ \.debug_loclists$/,$d' $B/firmware_v2.map | head -n 14 > forensic_map.out
rec forensic_map firmware_v2.map "$CLANGV" "first 14 lines of firmware_v2.map" 0
( timeout 3 $QEMU_ARM -M $MACH -nographic -monitor none -serial stdio -kernel $B/firmware_v2.elf > forensic_boot.out 2>&1 ) 2>/dev/null; rc=$?
rec forensic_boot firmware_v2.elf "$QEMUV" "timeout 3 qemu-system-arm -M $MACH -nographic -monitor none -serial stdio -kernel firmware_v2.elf" "$rc (134 = QEMU aborted itself after reporting a CPU lockup)" "$EMU"

# 5. forensic: the debugger's view of the core right after reset (QEMU's GDB stub as the probe)
S="$B/gdb.sock"
$QEMU_ARM -M $MACH -nographic -monitor none -serial null -kernel $B/firmware_v2.elf -S \
    -chardev socket,path=$S,server=on,wait=off,id=g0 -gdb chardev:g0 > $B/qemu.txt 2>&1 &
qpid=$!
sleep 1
timeout 30 lldb-18 --batch -o "process connect --plugin gdb-remote unix-connect://$S" \
    -o 'register read sp pc' -o 'memory read --size 4 --format x --count 4 0x0' \
    -o 'image lookup --address 0x08000000' -o 'kill' $B/firmware_v2.elf > forensic_reset.out 2>&1; rc=$?
kill $qpid 2>/dev/null; wait $qpid 2>/dev/null
sed -i -E -e "s#$(pwd)/##g" -e "s#unix-connect://[^ ]*#unix-connect://[socket path removed]#" -e '/^\(lldb\) kill$/,$d' forensic_reset.out
rec forensic_reset firmware_v2.elf "$LLDBV; $QEMUV" \
    "qemu-system-arm -M $MACH -S -gdb (unix socket) -kernel firmware_v2.elf; lldb-18 --batch: process connect, register read sp pc, memory read 0x0, image lookup" \
    "$rc" "$EMU; the host gdb in the container is x86-only, so LLDB 18 spoke the GDB remote protocol"
clean boot.out forensic_boot.out forensic_build.out build.out map.out forensic_map.out forensic_reset.out
rm -rf "$B"
exit $status
