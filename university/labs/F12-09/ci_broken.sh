#!/usr/bin/env bash
# ci_broken.sh - forensic evidence: the QEMU stage as the team first wrote it.
# It boots the kernel built by ci.sh with the command line of the commit under test.
# (Kept as evidence of a bad pattern. Do not copy it.)
B=${B:-.build}
cd "$B" || exit 2
echo "== stage qemu (old script)"
qemu-system-x86_64 -machine q35 -m 64M -display none -no-reboot -net none -serial stdio \
    -kernel kernel.elf -append "test=all,panic" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 | tee serial_ci.log
if [ $? -ne 0 ]; then
    echo "CI stage qemu: FAIL"; exit 1
fi
if grep -q "KTEST bitmap ok" serial_ci.log; then
    echo "CI stage qemu: PASS"
else
    echo "CI stage qemu: FAIL"; exit 1
fi
