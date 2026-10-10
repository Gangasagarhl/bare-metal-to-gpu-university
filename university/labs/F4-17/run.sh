#!/usr/bin/env bash
# F4-17 run.sh: stage (d), "design", for PCI device 1234:11e8: the edu4 driver written from the
# design note (design_note.txt), which was written from F4-16's observed specification.
#   build      the lab kernel with the edu4 driver (f417_main.cc, edu4.cc)
#   driver     bind, probe and use it: n! for n = 0..14
#   removal    fault injection: memory decoding switched off before n = 4 (expected: clean stop)
#   forensic   driver version 1 (-DF417_V1, no presence check) under the same fault
# regs_doc_check.cpp (design note complete? every register listed with a source?) is run by
# run_lab.sh itself.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
SRC="$B/boot.S $B/k4.cc $B/pci4.cc edu4.cc f417_main.cc"

kbuild k417.elf $SRC > build.out 2>&1; rc=$?
[ -s build.out ] || echo "(no messages: no warnings, no errors)" > build.out
rec build "boot.S k4.cc pci4.cc (from F4-14), edu4.cc, f417_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k417.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot driver.out 60 k417.elf -device edu; rc=$?
rec driver "f417_main.cc + edu4.cc (kernel k417.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k417.elf -device edu" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

KEXTRA="-DF417_REMOVE_AT=4" kbuild k417r.elf $SRC > .kbr.txt 2>&1 || status=1
qboot removal.out 60 k417r.elf -device edu; rc=$?
rec removal "f417_main.cc + edu4.cc built with -DF417_REMOVE_AT=4" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k417r.elf -device edu" "$rc" \
    "note:      exit code 35 (isa-debug-exit 0x11: one problem reported) is the expected, clean result of this fault" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

KEXTRA="-DF417_REMOVE_AT=4 -DF417_V1" kbuild k417v1.elf $SRC > .kbv.txt 2>&1 || status=1
qboot forensic.out 60 k417v1.elf -device edu; rc=$?
rec forensic "f417_main.cc + edu4.cc built with -DF417_REMOVE_AT=4 -DF417_V1 (driver version 1)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k417v1.elf -device edu" "$rc" \
    "note:      exit code 35: the kernel's own result check failed (expected for this forensic evidence)" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

rm -f k417.elf k417r.elf k417v1.elf .kbr.txt .kbv.txt
exit $status
