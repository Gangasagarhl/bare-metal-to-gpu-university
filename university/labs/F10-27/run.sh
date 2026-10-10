#!/usr/bin/env bash
# F10-27 lab steps: build the SAME vehicle code (vehicle.h) for the emulated microcontroller
# with the uRTOS backend (vehicle_fw.cc), run it in QEMU, compare its vehicle lines with the
# host backend's output (vehicle_host.out, made by run_lab.sh just before this script), report
# the image size, then build the forensic variant with a larger telemetry line buffer.
# uRTOS files (startup.cc, board.h, urtos.h, urtos.cc, os305.ld) are copies of labs/F3-39.
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
QEMU="qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -semihosting-config enable=on,target=native -icount shift=0,sleep=off"
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU mps2-an385 (Cortex-M3) with -icount shift=0,sleep=off; not ChibiOS and not a flight controller"
build() {  # build <elf> [extra flags]
    clang++ $ARMFLAGS ${2:-} -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS ${2:-} -c urtos.cc -o $B/urtos.o &&
    clang++ $ARMFLAGS ${2:-} -c vehicle_fw.cc -o $B/vehicle_fw.o &&
    ld.lld -T os305.ld --gc-sections $B/startup.o $B/urtos.o $B/vehicle_fw.o -o "$B/$1"
}

# 1. the vehicle code on the emulated microcontroller
{ build fw.elf && timeout 30 $QEMU -kernel $B/fw.elf; } > fw.out 2>&1; rc=$?
rec fw "startup.cc urtos.h urtos.cc board.h hal.h sim_airframe.h vehicle.h vehicle_fw.cc os305.ld" "$TC" \
    "clang++ $ARMFLAGS -c startup.cc urtos.cc vehicle_fw.cc; ld.lld -T os305.ld --gc-sections ... -o fw.elf; timeout 30 $QEMU -kernel fw.elf" \
    "$rc (0 = the report ended the run through semihosting with success)" "$HW"
[ "$rc" = 0 ] || status=1
grep -q "stack check: PASS" fw.out || status=1

# 2. same vehicle code, two backends: compare the lines the vehicle printed ("V ...")
if [ -f vehicle_host.out ]; then
    grep '^V ' vehicle_host.out > $B/host_v.txt
    grep '^V ' fw.out > $B/fw_v.txt
    if cmp -s $B/host_v.txt $B/fw_v.txt; then
        echo "vehicle lines identical on both backends ($(wc -l < $B/host_v.txt) lines)"
    else
        echo "vehicle lines DIFFER:"; diff $B/host_v.txt $B/fw_v.txt
    fi > compare.out
else
    echo "vehicle_host.out missing: run run_lab.sh, which builds the host listing first" > compare.out
fi
rec compare "vehicle_host.out fw.out" "grep, cmp (GNU coreutils/diffutils)" "grep '^V ' vehicle_host.out / fw.out, then cmp" 0
grep -q identical compare.out || status=1

# 3. memory cost of the firmware image
{
    llvm-size -A $B/fw.elf | grep -E '^(section|\.text|\.data|\.bss)'
    echo "--- largest symbols (llvm-nm --size-sort, top 6) ---"
    llvm-nm --size-sort -S -C $B/fw.elf | tail -n 6
} > size.out 2>&1; rc=$?
rec size fw.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A fw.elf; llvm-nm --size-sort -S -C fw.elf | tail -n 6" "$rc"

# 4. forensic: the same firmware with a 1400-byte telemetry line buffer
{ build forensic.elf -DLOG_LINE_BYTES=1400 && timeout 30 $QEMU -kernel $B/forensic.elf; } > forensic.out 2>&1; rc=$?
rec forensic "same sources, built with -DLOG_LINE_BYTES=1400" "$TC" \
    "clang++ $ARMFLAGS -DLOG_LINE_BYTES=1400 -c ...; ld.lld ...; timeout 30 $QEMU -kernel forensic.elf" \
    "$rc (1 = the report ended the run through semihosting with failure; expected for this variant)" "$HW"
[ "$rc" = 1 ] || status=1
# 5. sweep: the telemetry thread's report for several buffer sizes
for n in 64 600 948 1000 1400; do
    if build sweep.elf -DLOG_LINE_BYTES=$n; then
        printf 'LOG_LINE_BYTES=%-5s ' "$n"
        timeout 30 $QEMU -kernel $B/sweep.elf | grep -E '^task telem' | sed 's/^task telem: runs 4, //'
    else
        echo "LOG_LINE_BYTES=$n: build failed"
    fi
done > sweep.out 2>&1
rec sweep "same sources, built with -DLOG_LINE_BYTES=64, 600, 948, 1000, 1400" "$TC" \
    "for each n: clang++ $ARMFLAGS -DLOG_LINE_BYTES=n -c ...; ld.lld ...; timeout 30 $QEMU -kernel sweep.elf | grep '^task telem'" 0 "$HW"
[ "$(grep -c 'high-water' sweep.out)" = 5 ] || status=1
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' fw.out forensic.out
rm -rf "$B"
exit $status
