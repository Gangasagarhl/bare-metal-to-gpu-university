#!/usr/bin/env bash
# F4-16 run.sh: stage (c), lawful observation, on a device whose documentation this build
# could not open (PCI 1234:11e8 in QEMU).
#   build       the lab kernel: read-only dumps around the "working driver"'s self-test
#   observe     boot it with QEMU's memory-access trace on (-trace memory_region_ops_*)
#   trace       the trace filtered to the device's BAR0 region ("edu-mmio")
#   annotate    annotate.cc on that trace: sequence, per-register behaviour, cross effects
#   forensic    the learner's driver (built with -DF416_FORGET_ACK): its run, its trace, and
#               trace_diff.py against the working driver's trace (difference expected)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
SRC="$B/boot.S $B/k4.cc $B/pci4.cc vendor_drv.cc f416_main.cc"

kbuild k416.elf $SRC > build.out 2>&1; rc=$?
[ -s build.out ] || echo "(no messages: no warnings, no errors)" > build.out
rec build "boot.S k4.cc pci4.cc (from F4-14), vendor_drv.cc, f416_main.cc" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k416.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

TR="-trace memory_region_ops_read -trace memory_region_ops_write"
timeout 60 $QEMU -machine q35 $QCOMMON -serial file:.obs.txt -kernel k416.elf -device edu $TR -D .trace_full.txt
rc=$?; sed 's/\r$//' .obs.txt > observe.out
rec observe "f416_main.cc + vendor_drv.cc (kernel k416.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial file:observe.out -kernel k416.elf -device edu $TR -D trace_full.txt" "$rc" \
    "note:      exit code 33 = self-test passed; trace_full.txt had $(wc -l < .trace_full.txt) lines (every emulated region, firmware included), not kept" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

BASE=$(grep -m1 'edu-mmio' .trace_full.txt | sed -E 's/.*addr 0x([0-9a-f]+).*/\1/')
python3 trace_filter.py .trace_full.txt edu-mmio "$BASE" > trace.out; rc=$?
rec trace "trace_filter.py" "$PY_VER; $QEMU_VER" "python3 trace_filter.py trace_full.txt edu-mmio $BASE" "$rc" \
    "note:      BAR0 base 0x$BASE taken from the first edu-mmio access in the trace" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

hostbuild .annotate annotate.cc && ./.annotate < trace.out > annotate.out 2>&1; rc=$?
rec annotate "annotate.cc" "$GXX_VER" "g++ $HOSTFLAGS annotate.cc -o annotate; ./annotate < trace.out" "$rc"
[ "$rc" = 0 ] || status=1

KEXTRA="-DF416_FORGET_ACK" kbuild k416f.elf $SRC > .kbf.txt 2>&1 || status=1
timeout 60 $QEMU -machine q35 $QCOMMON -serial file:.fobs.txt -kernel k416f.elf -device edu $TR -D .ftrace_full.txt
rc=$?; sed 's/\r$//' .fobs.txt > forensic_run.out
rec forensic_run "f416_main.cc + vendor_drv.cc built with -DF416_FORGET_ACK (the learner's driver)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial file:forensic_run.out -kernel k416f.elf -device edu $TR -D ftrace_full.txt" "$rc" \
    "note:      exit code 35 = the kernel reported a failed self-test (isa-debug-exit 0x11); expected here" "$HW_NOTE"
[ "$rc" = 35 ] || status=1
FBASE=$(grep -m1 'edu-mmio' .ftrace_full.txt | sed -E 's/.*addr 0x([0-9a-f]+).*/\1/')
python3 trace_filter.py .ftrace_full.txt edu-mmio "$FBASE" > .ftrace.txt
python3 trace_diff.py trace.out .ftrace.txt > forensic_diff.out 2>&1; rc=$?
rec forensic_diff "trace_diff.py" "$PY_VER" "python3 trace_filter.py ftrace_full.txt edu-mmio $FBASE > ftrace.txt; python3 trace_diff.py trace.out ftrace.txt" "$rc" \
    "note:      exit code 1 = the traces differ, which is the expected result for this forensic evidence"
[ "$rc" = 1 ] || status=1

rm -f k416.elf k416f.elf .kbf.txt .obs.txt .fobs.txt .trace_full.txt .ftrace_full.txt .ftrace.txt .annotate
exit $status
