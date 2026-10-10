#!/usr/bin/env bash
# F9-51 lab steps:
#  1. build imu_node (uRTOS from F3-39 + imu_node.cc) for the emulated Cortex-M3 and run it in
#     QEMU with a deterministic virtual clock; the UART output is saved as firmware.out;
#  2. build the host agent (agent.cc) with sanitizers and feed it firmware.out;
#  3. run the firmware a second time and compare (the run must repeat exactly);
#  4. forensic variant: the same source built with a 4-slot ring and a 20-tick publisher;
#  5. image size.
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
HW="hardware:  untested on hardware; QEMU mps2-an385 with -icount shift=0,sleep=off (repeatable virtual time, not the timing of a real chip); the IMU is simulated in software"
SAN="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
build() {  # build <elf> [extra flags]
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c urtos.cc -o $B/urtos.o &&
    clang++ $ARMFLAGS ${2:-} -c imu_node.cc -o $B/imu_node.o &&
    ld.lld -T os305.ld --gc-sections $B/startup.o $B/urtos.o $B/imu_node.o -o "$B/$1"
}

# 1. firmware
{ build imu_node.elf && timeout 30 $QEMU -kernel $B/imu_node.elf; } > firmware.out 2>&1; rc=$?
rec firmware "startup.cc urtos.h urtos.cc board.h os305.ld imu_node.cc" "$TC" \
    "clang++ $ARMFLAGS -c startup.cc urtos.cc imu_node.cc; ld.lld -T os305.ld --gc-sections ... -o imu_node.elf; timeout 30 $QEMU -kernel imu_node.elf" \
    "$rc (0 = the report ended the run through semihosting)" "$HW"
[ "$rc" = 0 ] || status=1

# 2. host agent
g++ $SAN agent.cc -o $B/agent && $B/agent < firmware.out > agent.out 2>&1; rc=$?
rec agent agent.cc "$(g++ --version | head -n 1)" "g++ $SAN agent.cc -o agent; ./agent < firmware.out" "$rc"
[ "$rc" = 0 ] || status=1
grep -q "sequence gaps 0" agent.out || status=1

# 3. repeatability
timeout 30 $QEMU -kernel $B/imu_node.elf > $B/again.txt 2>&1
if cmp -s firmware.out $B/again.txt; then echo "second run: identical output ($(wc -l < firmware.out) lines)"; else echo "second run: DIFFERENT"; fi > repeat.out
rec repeat imu_node.elf "$(qemu-system-arm --version | head -n 1)" "timeout 30 $QEMU -kernel imu_node.elf, then cmp with firmware.out" 0 "$HW"
grep -q identical repeat.out || status=1

# 4. forensic variant
FX="-DRING_SIZE=4 -DPUB_PERIOD=20"
{ build imu_node_v2.elf "$FX" && timeout 30 $QEMU -kernel $B/imu_node_v2.elf; } > $B/fw2.txt 2>&1; rc=$?
$B/agent < $B/fw2.txt > forensic_agent.out 2>&1
rec forensic_agent "imu_node.cc built with $FX; agent.cc" "$TC" \
    "clang++ $ARMFLAGS $FX -c imu_node.cc; (link as step 1); qemu ... -kernel imu_node_v2.elf | ./agent" "$rc" "$HW"
[ "$rc" = 0 ] || status=1
grep -q "GAP" forensic_agent.out || status=1
grep '^R ' $B/fw2.txt > forensic_report.out
rec forensic_report "imu_node.cc built with $FX" "$TC" "grep '^R ' on the variant's UART output" 0 "$HW"

# 5. size
{
    llvm-size -A $B/imu_node.elf | grep -E '^(section|\.text|\.data|\.bss)'
} > size.out 2>&1; rc=$?
rec size imu_node.elf "$(llvm-size --version | grep -m1 'LLVM version' | sed 's/^ *//')" "llvm-size -A imu_node.elf" "$rc"
rm -rf "$B"
exit $status
