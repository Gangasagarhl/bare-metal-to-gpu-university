#!/usr/bin/env bash
# F3-25 run.sh: milestone B8 (timers and time keeping). Steps:
#   hpet      : HPET clock source; 10 s sleep timed by the host; 10 million monotonic reads
#   pit       : the same kernel with QEMU's HPET switched off: PIT fallback must still pass
#   forensic  : a kernel whose PIT calibration window overflows 16 bits (HPET off)
# The 1 % tolerance of the milestone is meant for KVM; this container has only TCG, so the
# verdict printed by hostclock.py is the TCG result, logged separately as the milestone asks.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc \
../F3-19/gdt.cc ../F3-19/isr.S ../F3-19/interrupts.cc ../F3-20/pmm.cc ../F3-21/paging.cc \
../F3-23/fb.cc ../F3-23/log.cc ../F3-24/acpi.cc ../F3-24/apic.cc ktime.cc b8_main.cc"
B=../F3-18/kbuild.sh
$B k_b8 "" $SRC && $B k_bug "-DB8_PIT_BUG" $SRC || exit 1
QB="-m 128M -nodefaults -display none -no-reboot -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04"
strip_boot() { grep -v -e '\]   0000' -e 'firmware memory map' -e 'usable ranges' -e '\] gdt:' -e '\] idt:' -e '\] paging:' "$1"; }
NOTE="note:      boot lines already shown in F3-19 to F3-21 (memory map, gdt, idt, paging) are left out"
TCG="note:      exit code 33 = B8 ok; the host-time comparison is TCG only (no KVM here), see the last two lines"

run_case() {   # <name> <machine> <kernel> <listing note>
    python3 -I hostclock.py 1 timeout 120 $QEMU -machine "$2" $QB -kernel "$3" > ".$1.txt" 2>&1
    local rc=$?
    strip_boot ".$1.txt" > "$1.out"
    rec "$1" "$4" "$QEMU_VER; $GXX_VER" "python3 -I hostclock.py 1 timeout 120 $QEMU -machine $2 $QB -kernel $3" "$rc" \
        "$TCG" "$NOTE" "$HW_NOTE"
    return $rc
}
within() {     # the TCG difference must stay under 5 % for the lab to count as working
    awk '/differ by/ { v = $7; sub(/%/, "", v); v = v < 0 ? -v : v; exit !(v < 5) }' "$1"
}

# 1. acceptance tests 1 and 2: 10 s sleep against the host clock; monotonic reads
run_case hpet pc k_b8.bin "b8_main.cc, ktime.cc, clock.h, timerq.h, hostclock.py"
[ $? = 33 ] && grep -q 'B8 ok' hpet.out && within hpet.out || status=1
# 2. acceptance test 3: HPET disabled, PIT fallback
run_case pit pc,hpet=off k_b8.bin "b8_main.cc, ktime.cc (PIT fallback path)"
[ $? = 33 ] && grep -q 'B8 ok' pit.out && within pit.out || status=1
# 3. forensic evidence: the same run with the calibration bug
run_case forensic pc,hpet=off k_bug.bin "b8_main.cc, ktime.cc built with -DB8_PIT_BUG"
[ $? = 33 ] || status=1

rm -f k_b8.* k_bug.* .hpet.txt .pit.txt .forensic.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
