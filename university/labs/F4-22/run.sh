#!/usr/bin/env bash
# F4-22 run.sh: idle in a real (emulated) kernel, measured from outside.
#   build      three variants of the idle kernel (irq.S, f422_main.cc + F4-14 base)
#   idle_halt  the governor version: halts between timer ticks
#   idle_poll  -DF422_FORCE_POLL: spins between timer ticks
#   forensic   -DF422_UNIT_BUG: the governor's unit conversion is wrong
# measure.py reports host CPU time against wall time for each QEMU run. governor.cpp,
# thermal.cpp and power_readout.cpp are run by run_lab.sh itself.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
SRC="$B/boot.S $B/k4.cc irq.S f422_main.cc"
QRUN="timeout 60 $QEMU -machine q35 $QCOMMON -serial stdio -kernel"
NOTE="note:      exit code 33 = pass; host CPU time is measured with getrusage on the QEMU process (TCG: the guest CPU is a host thread); it is not a power measurement and varies between runs"

: > build.out; rc=0
kbuild k422h.elf $SRC >> build.out 2>&1 || rc=1
KEXTRA="-DF422_FORCE_POLL" kbuild k422p.elf $SRC >> build.out 2>&1 || rc=1
KEXTRA="-DF422_UNIT_BUG" kbuild k422b.elf $SRC >> build.out 2>&1 || rc=1
[ -s build.out ] || echo "(no messages: no warnings, no errors)" > build.out
rec build "boot.S k4.cc (F4-14), irq.S, f422_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS [ -DF422_FORCE_POLL | -DF422_UNIT_BUG ] -c <each file>; ld -m elf_i386 -T kernel.ld" "$rc"
[ "$rc" = 0 ] || status=1

python3 measure.py idle_halt $QRUN k422h.elf > idle_halt.out 2>&1; rc=$?
rec idle_halt "f422_main.cc (kernel k422h.elf) under measure.py" "$QEMU_VER; $PY_VER" "python3 measure.py idle_halt $QRUN k422h.elf" "$rc" "$NOTE" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 measure.py idle_poll $QRUN k422p.elf > idle_poll.out 2>&1; rc=$?
rec idle_poll "f422_main.cc -DF422_FORCE_POLL (kernel k422p.elf) under measure.py" "$QEMU_VER; $PY_VER" "python3 measure.py idle_poll $QRUN k422p.elf" "$rc" "$NOTE" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 measure.py forensic $QRUN k422b.elf > forensic.out 2>&1; rc=$?
rec forensic "f422_main.cc -DF422_UNIT_BUG (kernel k422b.elf) under measure.py" "$QEMU_VER; $PY_VER" "python3 measure.py forensic $QRUN k422b.elf" "$rc" "$NOTE" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f k422h.elf k422p.elf k422b.elf
exit $status
