#!/usr/bin/env bash
# F4-18 run.sh: stage (e), "test", for the F4-17 driver (test_plan.txt lists every test).
#   unit        host unit and fault-injection tests: edu4.cc compiled unchanged with host shims
#   golden      QEMU records the driver's register accesses during the F4-17 run
#   replay      that recording played back to the host build of the driver
#   absent      QEMU without the device: the driver must not bind (and nothing hangs)
#   soak        QEMU, 20,000 computations
#   mutation    the suite run against a mutated copy of edu4.cc (busy-wait removed)
#   forensic    the weak suite on the mutant (the CI log) and the mutant kernel in QEMU (the field)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0
B=../F4-14
D=../F4-17
HOSTX="-Ihost -I. -I$D"

hostbuild .test_edu4 $HOSTX test_edu4.cc shim_host.cc $D/edu4.cc > .b1.txt 2>&1 && ./.test_edu4 > unit.out 2>&1; rc=$?
rec unit "test_edu4.cc + shim_host.cc + F4-17/edu4.cc (unchanged)" "$GXX_VER" \
    "g++ $HOSTFLAGS -Ihost -I. -I../F4-17 test_edu4.cc shim_host.cc ../F4-17/edu4.cc -o test_edu4; ./test_edu4" "$rc"
[ "$rc" = 0 ] || { status=1; cat .b1.txt; }

kbuild k417.elf $B/boot.S $B/k4.cc $B/pci4.cc $D/edu4.cc $D/f417_main.cc > .kb.txt 2>&1 || { status=1; cat .kb.txt; }
TR="-trace memory_region_ops_read -trace memory_region_ops_write"
timeout 60 $QEMU -machine q35 $QCOMMON -serial file:.golden_serial.txt -kernel k417.elf -device edu $TR -D .gtrace.txt; rc=$?
GB=$(grep -m1 'edu-mmio' .gtrace.txt | sed -E 's/.*addr 0x([0-9a-f]+).*/\1/')
python3 ../F4-16/trace_filter.py .gtrace.txt edu-mmio "$GB" > golden_trace.out
rec golden_trace "F4-17 kernel (f417_main.cc + edu4.cc), F4-16/trace_filter.py" "$QEMU_VER; $PY_VER" \
    "$QEMU -machine q35 $QCOMMON -serial file:serial.txt -kernel k417.elf -device edu $TR -D gtrace.txt; python3 trace_filter.py gtrace.txt edu-mmio $GB" \
    "$rc" "note:      exit code 33 = the F4-17 run passed while it was recorded; $(wc -l < golden_trace.out) accesses kept" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

hostbuild .test_replay $HOSTX test_replay.cc shim_host.cc $D/edu4.cc > .b2.txt 2>&1 && ./.test_replay golden_trace.out > replay.out 2>&1; rc=$?
rec replay "test_replay.cc + shim_host.cc + F4-17/edu4.cc" "$GXX_VER" \
    "g++ $HOSTFLAGS -Ihost -I. -I../F4-17 test_replay.cc shim_host.cc ../F4-17/edu4.cc -o test_replay; ./test_replay golden_trace.out" "$rc"
[ "$rc" = 0 ] || { status=1; cat .b2.txt; }

qboot absent.out 60 k417.elf; rc=$?
rec absent "F4-17 kernel (f417_main.cc + edu4.cc), no -device edu" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k417.elf" "$rc" \
    "note:      exit code 35 (isa-debug-exit 0x11) with \"edu4: not bound\" is the expected result of test E2" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

KEXTRA="-I$D" kbuild k418s.elf $B/boot.S $B/k4.cc $B/pci4.cc $D/edu4.cc f418_soak.cc > .ks.txt 2>&1 || { status=1; cat .ks.txt; }
qboot soak.out 300 k418s.elf -device edu; rc=$?
rec soak "f418_soak.cc + F4-17/edu4.cc (kernel k418s.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k418s.elf -device edu" "$rc" "note:      exit code 33 = pass" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -rf .mut; mkdir .mut; cp $D/edu4.h $D/edu_regs.h .mut/
sed 's/if (!(st \& kStatusBusy)) break;/break;   \/\/ MUTANT: busy-wait removed/' $D/edu4.cc > .mut/edu4.cc
hostbuild .t_mut -Ihost -I. -I.mut test_edu4.cc shim_host.cc .mut/edu4.cc > .b3.txt 2>&1 || { status=1; cat .b3.txt; }
{
    echo "mutant: $(diff $D/edu4.cc .mut/edu4.cc | grep '^>' | sed 's/^> *//')"
    for suite in "--weak" ""; do
        for build in original mutant; do
            bin=./.test_edu4; [ "$build" = mutant ] && bin=./.t_mut
            $bin $suite > .r.txt 2>&1; r=$?
            printf '%-12s suite on the %-8s driver: exit %d; %s\n' "${suite:---full}" "$build" "$r" "$(tail -n 1 .r.txt)"
            grep FAIL .r.txt | sed 's/^/      /' || true
        done
    done
} > mutation.out 2>&1; rc=$?
rec mutation "test_edu4.cc on F4-17/edu4.cc and on a mutated copy (sed: busy-wait removed)" "$GXX_VER" \
    "sed 's/if (!(st & kStatusBusy)) break;/break;/' edu4.cc > mut/edu4.cc; build test_edu4 against each; run --weak and full" "$rc"
[ "$rc" = 0 ] || status=1

./.t_mut --weak > forensic_ci.out 2>&1; rc=$?
rec forensic_ci "test_edu4.cc --weak on the shipped (mutated) driver" "$GXX_VER" "./test_edu4 --weak" "$rc" \
    "note:      exit code 0: the weak suite passes on the faulty driver (this is the evidence)"
[ "$rc" = 0 ] || status=1
kbuild k418m.elf $B/boot.S $B/k4.cc $B/pci4.cc .mut/edu4.cc $D/f417_main.cc > .km.txt 2>&1 || { status=1; cat .km.txt; }
qboot forensic_field.out 60 k418m.elf -device edu; rc=$?
rec forensic_field "F4-17 kernel built with the mutated edu4.cc" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k418m.elf -device edu" "$rc" \
    "note:      exit code 35: the kernel's own value check failed (expected for this forensic evidence)" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

rm -rf .mut k417.elf k418s.elf k418m.elf .test_edu4 .test_replay .t_mut .b1.txt .b2.txt .b3.txt .kb.txt .ks.txt .km.txt \
    .golden_serial.txt .gtrace.txt .r.txt
exit $status
