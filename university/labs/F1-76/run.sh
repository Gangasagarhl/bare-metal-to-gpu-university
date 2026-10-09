#!/usr/bin/env bash
# F1-76 lab steps that run_lab.sh cannot do alone: the forensic run of the PWM model, and
# the bare-metal blink + software PWM firmware booted in QEMU (mps2-an385) with QEMU's
# trace of every write to the LED register and to the timer.
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
# 1. forensic: Listing 1 on Tomas's settings
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
g++ $FLAGS pwm.cpp -o $B/pwm || status=1
$B/pwm < forensic.in > forensic.out 2>&1; rc=$?
rec forensic "pwm.cpp, input forensic.in" "$(g++ --version | head -n 1)" "./pwm < forensic.in" "$rc"
[ "$rc" = 0 ] || status=1

# 2. firmware: blink and software PWM with our own start-up code
ARMFLAGS="--target=thumbv7m-none-eabi -mcpu=cortex-m3 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -Os -g -Wall -Wextra -Wpedantic -Werror"
QEMUCMD="qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio -trace mps2_fpgaio_write -trace cmsdk_apb_timer_write -D trace.txt -kernel blink.elf"
cmd="clang++ $ARMFLAGS -c startup.cc blink.cc && ld.lld -T mps2_an385.ld --gc-sections startup.o blink.o -o blink.elf && timeout 5 $QEMUCMD"
{
    clang++ $ARMFLAGS -c startup.cc -o $B/startup.o &&
    clang++ $ARMFLAGS -c blink.cc -o $B/blink.o &&
    ld.lld -T mps2_an385.ld --gc-sections $B/startup.o $B/blink.o -o $B/blink.elf &&
    llvm-size -B $B/blink.elf | sed "s#$B/##" > size.out &&
    timeout 5 qemu-system-arm -M mps2-an385 -nographic -monitor none -serial stdio \
        -trace mps2_fpgaio_write -trace cmsdk_apb_timer_write -D $B/trace.txt -kernel $B/blink.elf
} > blink.out 2>&1; rc=$?
sed -i -E 's/from pid [0-9]+/from pid [pid removed]/' blink.out
TC="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1); $(qemu-system-arm --version | head -n 1)"
HW="hardware:  untested on hardware; QEMU mps2-an385 model: the LED is a register bit, no real LED was driven"
rec blink "startup.cc blink.cc mps2_an385.ld" "$TC" "$cmd" "$rc (124 = stopped by the 5 s time limit; the firmware loops forever by design)" "$HW"
rec size blink.elf "$(llvm-size --version | sed -n 2p | sed 's/^ *//')" "llvm-size -B blink.elf" 0
grep -q "^done" blink.out || status=1
# QEMU's trace: the first timer writes (configuration) and the LED writes of the blink phase
{
    echo "--- timer register writes other than flag clears (the configuration) ---"
    grep cmsdk_apb_timer_write $B/trace.txt | grep -v 'offset 0xc data 0x1' | head -n 8
    echo "--- LED register writes, first 8 ---"
    grep mps2_fpgaio_write $B/trace.txt | head -n 8
    echo "--- counts ---"
    echo "LED register writes in total: $(grep -c mps2_fpgaio_write $B/trace.txt)"
    echo "timer flag clears (INTCLEAR writes): $(grep -c 'offset 0xc data 0x1' $B/trace.txt)"
} > trace.out
rec trace "QEMU trace of the blink run" "$(qemu-system-arm --version | head -n 1)" \
    "-trace mps2_fpgaio_write -trace cmsdk_apb_timer_write -D trace.txt (same run as blink)" 0 "$HW"
rm -rf "$B"
exit $status
