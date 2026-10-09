#!/usr/bin/env bash
# F3-19 run.sh: milestone B2. Builds three variants of the test kernel and boots each in QEMU:
#   exceptions  : every test exception is reported, the stack overflow ends on the IST stack
#   intlog      : a normal boot under QEMU's interrupt log ("-d int"): no exception expected
#   forensic_*  : the same tests with no IST stack for #DF: QEMU's log ends in a triple fault
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
S18="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc"
SRC="$S18 gdt.cc isr.S interrupts.cc b2_main.cc"
B=../F3-18/kbuild.sh
$B k_b2 "-DB2_TESTS" $SRC && $B k_quiet "" $SRC && $B k_noist "-DB2_FORGET_IST" $SRC || exit 1

# 1. acceptance tests 1 and 2: the exception tests and the double fault on the IST stack
qrun exceptions.out 30 k_b2.bin; rc=$?
rec exceptions "b2_main.cc built with -DB2_TESTS (kbuild.sh)" "$QEMU_VER; $GXX_VER" \
    "$QEMU $QBASE -serial stdio -kernel k_b2.bin" "$rc" "note:      exit code 33 = pass (written by the double-fault test after its report)" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q 'B2 ok' exceptions.out || status=1

# 2. acceptance test 3: QEMU's interrupt log during a normal boot
qrun .serial.txt 30 k_quiet.bin -d int -D .int.txt; rc=$?
n=$(grep -c ' v=' .int.txt)
{
    echo "== serial output =="
    cat .serial.txt
    echo "== QEMU interrupt log (-d int): exceptions and interrupts delivered during the boot =="
    echo "lines with ' v=' (one per delivered exception or interrupt): $n"
    grep ' v=' .int.txt | head -n 5
} > intlog.out
rec intlog "b2_main.cc built without test flags" "$QEMU_VER" \
    "$QEMU $QBASE -serial stdio -kernel k_quiet.bin -d int -D int.txt" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] && [ "$n" = 0 ] || status=1

# 3. forensic evidence: no IST for #DF -> triple fault (QEMU stops because of -no-reboot)
qrun forensic_serial.out 30 k_noist.bin -d int,cpu_reset -D .int2.txt; rc=$?
total=$(wc -l < .int2.txt)
first=$(grep -n 'check_exception' .int2.txt | tail -n 3 | head -n 1 | cut -d: -f1)
{
    echo "== QEMU log (-d int,cpu_reset), lines $first-$total of $total: the last three exceptions =="
    sed -n "${first},${total}p" .int2.txt
} > forensic_int.out
rec forensic_serial "b2_main.cc built with -DB2_FORGET_IST" "$QEMU_VER" \
    "$QEMU $QBASE -serial stdio -kernel k_noist.bin -d int,cpu_reset -D qemu.log" "$rc" \
    "note:      exit code 0: with -no-reboot, QEMU ends the run when the guest resets after the triple fault" "$HW_NOTE"
rec forensic_int "QEMU's own log of the same run" "$QEMU_VER" "(same run; excerpt of qemu.log)" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] && grep -q 'Triple fault' forensic_int.out || status=1

rm -f k_b2.* k_quiet.* k_noist.* .serial.txt .int.txt .int2.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
