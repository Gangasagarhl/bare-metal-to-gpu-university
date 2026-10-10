#!/usr/bin/env bash
# F1-75 lab steps that run_lab.sh cannot do alone: run the clock-tree calculator on the
# forensic configuration, and boot the register probe on QEMU's STM32F100 model with
# QEMU's "unimplemented device" log switched on.
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
# 1. forensic: Listing 1 on Lena's configuration
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS clocktree.cpp -o $B/clocktree || status=1
$B/clocktree < forensic.in > forensic.out 2>&1; rc=$?
rec forensic "clocktree.cpp, input forensic.in" "$(g++ --version | head -n 1)" "./clocktree < forensic.in" "$rc"
[ "$rc" = 0 ] || status=1

# 2. register probe on the emulated STM32F100, with QEMU's unimplemented-device log
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
cmd="clang++ $ARMFLAGS -c startup.cc probe.cc && ld.lld -T stm32f100.ld --gc-sections startup.o probe.o -o probe.elf && timeout 3 qemu-system-arm -M stm32vldiscovery -nographic -monitor none -serial stdio -d unimp -kernel probe.elf"
{
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c probe.cc -o $B/probe.o &&
    ld.lld -T stm32f100.ld --gc-sections $B/startup.o $B/probe.o -o $B/probe.elf &&
    timeout 3 qemu-system-arm -M stm32vldiscovery -nographic -monitor none -serial stdio -d unimp -kernel $B/probe.elf
} > probe.out 2>&1; rc=$?
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' probe.out
rec probe "startup.cc probe.cc stm32f100.ld" \
    "$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)" \
    "$cmd" "$rc (124 = stopped by the 3 s time limit; the firmware loops forever by design)" \
    "hardware:  untested on hardware; QEMU's STM32F100 model does not implement RCC or GPIO (see its log lines)"
grep -q "^done" probe.out || status=1
rm -rf "$B"
exit $status
