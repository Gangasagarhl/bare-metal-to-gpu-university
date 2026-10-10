#!/usr/bin/env bash
# qemu_test.sh <name> <kernel command line> <time limit in s> <serial log file>
# Boots the test kernel once in QEMU and prints one verdict line. The rules come from the
# Systems Curriculum, section 0: serial output is the oracle, pass/fail comes from the
# isa-debug-exit status, every run has a wall-clock limit (a hang is a failure), and the
# machine is fixed (machine type, memory, CPU count) and recorded.
# Exit status: 0 = PASS, 1 = FAIL.
set -u
name=$1; cmdline=$2; limit=$3; log=$4
MACHINE="-machine q35 -m 64M -smp 1"
rm -f "$log"
timeout "$limit" qemu-system-x86_64 $MACHINE -display none -no-reboot -net none \
    -serial "file:$log" -kernel kernel.elf -append "$cmdline" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
rc=$?
if [ "$rc" = 124 ]; then
    verdict="FAIL (no result within $limit s: a hang is a failure)"
elif grep -q "PANIC" "$log"; then
    verdict="FAIL (PANIC in the serial log; QEMU exit status $rc)"
elif [ "$rc" = 33 ] && grep -q "^KTEST summary [0-9]* passed 0 failed" "$log"; then
    verdict="PASS"
else
    verdict="FAIL (QEMU exit status $rc; last serial line: $(tail -n 1 "$log"))"
fi
echo "qemu test $name [$MACHINE, limit $limit s, cmdline '$cmdline']: $verdict"
[ "$verdict" = PASS ]
