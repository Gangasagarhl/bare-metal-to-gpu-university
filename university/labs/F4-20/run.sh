#!/usr/bin/env bash
# F4-20 run.sh: a bring-up kernel and its timeline, on emulated PCs (no real PC in the build).
#   build       the bring-up kernel (f420_main.cc, smbios4.cc + F4-14/F4-15 parts)
#   boot        q35 with SMBIOS strings set on the command line (cross-check for smbios4.cc)
#   boot_nops2  q35 without a PS/2 controller (i8042=off), as many new PCs are
#   timeline    timeline.py: boot vs boot_nops2 (expected: no regression)
#   forensic    the "slow boot" case: a kernel without the presence check on the machine
#               without a PS/2 controller, then timeline.py boot_nops2 vs forensic (expected exit 1)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
SRC="$B/boot.S $B/k4.cc $B/pci4.cc $B/acpi4.cc ../F4-15/bind4.cc smbios4.cc f420_main.cc"
SMB="-smbios type=1,manufacturer=DR401-lab,product=spare-pc,version=1"
DEV="-device edu"

KEXTRA="-I../F4-15" kbuild k420.elf $SRC > build.out 2>&1; rc=$?
KEXTRA="-I../F4-15 -DF420_NO_PRESENCE_CHECK" kbuild k420f.elf $SRC >> build.out 2>&1 || rc=1
[ -s build.out ] || echo "(no messages: no warnings, no errors)" > build.out
rec build "boot.S k4.cc pci4.cc acpi4.cc (F4-14), bind4.cc (F4-15), smbios4.cc, f420_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -I../F4-15 [-DF420_NO_PRESENCE_CHECK] -c <each file>; ld -m elf_i386 -T kernel.ld -o k420.elf / k420f.elf" "$rc"
[ "$rc" = 0 ] || status=1

qboot boot.out 60 k420.elf $DEV $SMB; rc=$?
rec boot "f420_main.cc (kernel k420.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k420.elf $DEV $SMB" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10); ticks = time-stamp counter ticks, not converted" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

timeout 60 $QEMU -machine q35,i8042=off $QCOMMON -serial stdio -kernel k420.elf $DEV > boot_nops2.out 2>&1; rc=$?
sed -i 's/\r$//' boot_nops2.out
rec boot_nops2 "f420_main.cc (kernel k420.elf)" "$QEMU_VER" \
    "$QEMU -machine q35,i8042=off $QCOMMON -serial stdio -kernel k420.elf $DEV" "$rc" \
    "note:      exit code 33 = pass" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 timeline.py boot.out boot_nops2.out > timeline.out 2>&1; rc=$?
rec timeline "timeline.py" "$PY_VER" "python3 timeline.py boot.out boot_nops2.out" "$rc" \
    "note:      exit code 0 expected (no stage regressed)"
[ "$rc" = 0 ] || status=1

s=$(date +%s%N)
timeout 60 $QEMU -machine q35,i8042=off $QCOMMON -serial stdio -kernel k420f.elf $DEV > forensic_boot.out 2>&1; rc=$?
e=$(date +%s%N)
sed -i 's/\r$//' forensic_boot.out
rec forensic_boot "f420_main.cc built with -DF420_NO_PRESENCE_CHECK (kernel k420f.elf)" "$QEMU_VER" \
    "$QEMU -machine q35,i8042=off $QCOMMON -serial stdio -kernel k420f.elf $DEV" "$rc" \
    "note:      exit code 33: the boot completes, only slowly" \
    "wall:      $(( (e - s) / 1000000 )) ms for the whole QEMU run (host clock; TCG, so not a hardware time)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 timeline.py boot_nops2.out forensic_boot.out > forensic.out 2>&1; rc=$?
rec forensic "timeline.py" "$PY_VER" "python3 timeline.py boot_nops2.out forensic_boot.out" "$rc" \
    "note:      exit code 1 expected: the forensic boot has a regressed stage"
[ "$rc" = 1 ] || status=1

rm -f k420.elf k420f.elf
exit $status
