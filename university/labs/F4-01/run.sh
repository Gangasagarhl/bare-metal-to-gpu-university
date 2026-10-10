#!/usr/bin/env bash
# F4-01 run.sh: build the DR301 lab kernel with the driver model and boot it in QEMU.
#   build    : kernel build (g++ -m32 freestanding, GNU ld)
#   boot     : the binding run on QEMU's "pc" machine with one serial port (COM1 only)
#   boot2    : the same kernel with a second serial port added (-serial file for COM2)
#   forensic : the kernel built with F401_SKIP_PRESENCE_CHECK (the colleague's "fast probe")
set -u -o pipefail
cd "$(dirname "$0")"
. ./lablib.sh
status=0
SRC="boot.S kbase.cc driver.cc uart16550.cc f401_main.cc"

kbuild k401.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k401.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k401.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot boot.out 30 pc k401.elf; rc=$?
rec boot "f401_main.cc (kernel k401.elf)" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -kernel k401.elf" "$rc" \
    "note:      exit code 33 = pass: the kernel wrote 0x10 to isa-debug-exit and QEMU exits with (0x10 << 1) | 1" \
    "note:      -nodefaults: the machine has COM1 (from -serial stdio) and no COM2" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f .com2.txt
qboot .boot2.txt 30 pc k401.elf -serial file:.com2.txt; rc=$?
{ cat .boot2.txt; echo "== what arrived on COM2 (the file behind -serial file:com2.txt) =="; sed 's/\r$//' .com2.txt; } > boot2.out
rec boot2 "f401_main.cc (kernel k401.elf)" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -serial file:com2.txt -kernel k401.elf" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

KEXTRA="-DF401_SKIP_PRESENCE_CHECK" kbuild k401f.elf $SRC > .kbf.txt 2>&1 || status=1
qboot forensic.out 30 pc k401f.elf; rc=$?
rec forensic "f401_main.cc and uart16550.cc built with -DF401_SKIP_PRESENCE_CHECK" "$QEMU_VER" \
    "$QEMU -machine pc $QCOMMON -serial stdio -kernel k401f.elf" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f k401.elf k401f.elf .kb.txt .kbf.txt .boot2.txt .com2.txt
exit $status
