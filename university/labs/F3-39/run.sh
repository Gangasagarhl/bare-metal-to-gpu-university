#!/usr/bin/env bash
# F3-39 lab steps: build uRTOS with the three-task application, run it in QEMU with a
# deterministic virtual clock (-icount), run it a second time to check that the result
# repeats, build the forensic variant, and report sizes. rta.cpp is run by run_lab.sh.
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
HW="hardware:  untested on hardware; QEMU mps2-an385 with -icount shift=0,sleep=off (virtual time advances by a fixed amount per instruction, so the timing is repeatable but is not the timing of a real chip)"
build() {  # build <app source> <elf>
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c urtos.cc -o $B/urtos.o &&
    clang++ $ARMFLAGS -c "$1" -o $B/app.o &&
    ld.lld -T os305.ld --gc-sections $B/startup.o $B/urtos.o $B/app.o -o "$B/$2"
}

# 1. the rate-monotonic task set
{ build app.cc app.elf && timeout 30 $QEMU -kernel $B/app.elf; } > app.out 2>&1; rc=$?
rec app "startup.cc urtos.h urtos.cc app.cc board.h" "$TC" \
    "clang++ $ARMFLAGS -c startup.cc urtos.cc app.cc; ld.lld -T os305.ld --gc-sections ... -o app.elf; timeout 30 $QEMU -kernel app.elf" \
    "$rc (0 = the report task ended the run through semihosting)" "$HW"
grep -q "all deadlines met" app.out || status=1

# 2. repeatability: run the same image again and compare the two outputs byte for byte
timeout 30 $QEMU -kernel $B/app.elf > $B/again.txt 2>&1
if cmp -s app.out $B/again.txt; then echo "second run: identical output ($(wc -l < app.out) lines)"; else echo "second run: DIFFERENT"; diff app.out $B/again.txt; fi > repeat.out
rec repeat app.elf "$(qemu-system-arm --version | head -n 1)" "timeout 30 $QEMU -kernel app.elf, then cmp with app.out" 0 "$HW"
grep -q identical repeat.out || status=1

# 3. forensic: app_v2.cc (only the priorities differ)
{ build app_v2.cc app_v2.elf && timeout 30 $QEMU -kernel $B/app_v2.elf; } > forensic.out 2>&1; rc=$?
rec forensic "startup.cc urtos.cc app_v2.cc" "$TC" "same build as step 1 with app_v2.cc; timeout 30 $QEMU -kernel app_v2.elf" "$rc" "$HW"
grep -q "DEADLINE MISSED" forensic.out || status=1

# 4. what the kernel costs in memory
{
    llvm-size -A $B/app.elf | grep -E '^(section|\.text|\.data|\.bss)'
    echo "--- largest symbols (llvm-nm --size-sort, top 8) ---"
    llvm-nm --size-sort -S -C $B/app.elf | tail -n 8
} > size.out 2>&1; rc=$?
rec size app.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A app.elf; llvm-nm --size-sort -S -C app.elf | tail -n 8" "$rc"
rm -rf "$B"
exit $status
