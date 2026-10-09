#!/usr/bin/env bash
# F5-30 lab: what this build's QEMU offers for BMC work, and a first program on an emulated BMC SoC.
# 1. list the BMC machines QEMU knows (its own help output);
# 2. record the options of the ast2600-evb machine and its memory map (QEMU monitor "info mtree -f");
# 3. build bmc_hello.cc for Cortex-A7 with clang/LLD and run it on ast2600-evb.
# No OpenBMC image exists in this build (no internet): booting OpenBMC is untested (see F5-30).
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-arm
QEMUV="$($QEMU --version | head -n 1)"
CLANGV="$(clang++ --version | head -n 1); $(ld.lld --version | head -n 1)"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
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

# 1. BMC machine models
$QEMU -machine help > $B/machines.txt 2>&1; rc=$?
grep -i -E "bmc|aspeed|ast[0-9]|npcm" $B/machines.txt > bmc_machines.out
rec bmc_machines "(no listing: QEMU's own help output)" "$QEMUV" \
    "$QEMU -machine help | grep -i -E 'bmc|aspeed|ast[0-9]|npcm'" "$rc"

# 2. options and memory map of ast2600-evb (the monitor is fed two commands; the CPU never starts: -S)
$QEMU -machine ast2600-evb,help 2>&1 | grep -E "bmc-console|fmc-model|spi-model|execute-in-place|boot=|firmware=|kernel=" > $B/opts.txt; rc=$?
(echo "info mtree -f"; sleep 2; echo quit) | timeout 30 $QEMU -machine ast2600-evb -display none -S \
    -monitor stdio -serial null 2>&1 | tr -d '\r' > $B/mtree.txt
{
    echo "== selected options of the ast2600-evb machine ($QEMU -machine ast2600-evb,help)"
    cat $B/opts.txt
    echo
    echo "== selected regions of the CPU's flat memory view (info mtree -f, FlatView of 'cpu-memory-0')"
    awk '/AS "cpu-memory-0"/{f=1} f&&/FlatView/{exit} f' $B/mtree.txt |
        grep -E "\((prio 0|prio -1000), (ram|i/o)\): (ram|aspeed\.(scu|adc\.engine\.0|gpio|i2c\.bus\.0|smc|sdmc|lpc|pwm|timer)|serial|ftgmac100|aspeed-rtc)( |$)" |
        awk '!seen[$NF]++ || $NF=="serial"' | head -n 30
} > bmc_mtree.out
rec bmc_mtree "(no listing: QEMU's own help and monitor output, filtered with grep and awk)" "$QEMUV" \
    "$QEMU -machine ast2600-evb,help | grep ...; (echo 'info mtree -f'; echo quit) | $QEMU -machine ast2600-evb -display none -S -monitor stdio -serial null | awk/grep filter in run.sh" "$rc"

# 3. bare-metal program on the emulated BMC SoC
ARM_CXX="clang++ --target=armv7a-none-eabi -mcpu=cortex-a7 -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -O2 -Wall -Wextra -Wpedantic -Werror"
$ARM_CXX -c bmc_hello.cc -o $B/bmc_hello.o > $B/build.txt 2>&1 &&
    ld.lld -T bmc_hello.ld $B/bmc_hello.o -o $B/bmc_hello.elf >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
timeout 10 $QEMU -machine ast2600-evb -display none -monitor none -serial file:$B/uart.txt \
    -device loader,file=$B/bmc_hello.elf,cpu-num=0 > $B/qemu.txt 2>&1; rc=$?
cat $B/uart.txt > bmc_hello.out
rec bmc_hello "bmc_hello.cc bmc_hello.ld" "$CLANGV; $QEMUV" \
    "$ARM_CXX -c bmc_hello.cc && ld.lld -T bmc_hello.ld bmc_hello.o -o bmc_hello.elf; timeout 10 $QEMU -machine ast2600-evb -display none -monitor none -serial file:uart.txt -device loader,file=bmc_hello.elf,cpu-num=0" \
    "$rc (124 = stopped by the 10 s time limit: the program halts in a loop by design and QEMU has no exit device here; the UART file is the result)" \
    "hardware:  untested on hardware; QEMU 8.2.2 ast2600-evb machine model, not a real BMC"
grep -q "bmc_hello: done" bmc_hello.out || status=1

rm -rf "$B"
exit $status
