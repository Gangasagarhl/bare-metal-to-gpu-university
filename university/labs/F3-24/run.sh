#!/usr/bin/env bash
# F3-24 run.sh: milestone B7 (ACPI tables, local APIC, IOAPIC). Steps:
#   boot           : tables, MADT, APIC set-up, serial IRQ 4, keyboard IRQ 1 from monitor sendkey
#   x2apic_on/off  : the same kernel with QEMU's x2APIC CPU flag requested and removed
#   corrupt        : the test patch that damages the MADT in memory: the table must be rejected
#   forensic_*     : a keyboard handler without end-of-interrupt, and the monitor's view of the local APIC
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc \
../F3-19/gdt.cc ../F3-19/isr.S ../F3-19/interrupts.cc ../F3-20/pmm.cc ../F3-21/paging.cc \
../F3-23/fb.cc ../F3-23/log.cc acpi.cc apic.cc b7_main.cc"
B=../F3-18/kbuild.sh
$B k_b7 "" $SRC && $B k_corrupt "-DB7_CORRUPT" $SRC && $B k_noeoi "-DB7_NO_EOI" $SRC || exit 1
QM="python3 -I ../F3-23/qmp_cmds.py"
QV="-machine pc -m 128M -nodefaults -display none -no-reboot"
KEYS="sendkey a;sleep 1;sendkey b;sleep 2"
strip_boot() { grep -v -e '^  0000' -e '^firmware memory map' -e '^usable ranges' -e '^gdt:' -e '^idt:' -e '^paging:' "$1"; }
NOTE="note:      boot lines already shown in F3-19 to F3-21 (memory map, gdt, idt, paging) are left out"

# 1. acceptance tests 1 and 2: tables with checksums; IRQ 1 and IRQ 4 through the IOAPIC
$QM .s1.txt "waiting for keys" "$KEYS" $QEMU $QV -kernel k_b7.bin > .q1.txt 2>&1; rc=$?
{ strip_boot .s1.txt; echo "== QEMU monitor (QMP) =="; cat .q1.txt; } > boot.out
rec boot "b7_main.cc, acpi.cc, madt.h, apic.cc" "$QEMU_VER; $GXX_VER" \
    "python3 -I qmp_cmds.py serial.txt 'waiting for keys' '$KEYS' $QEMU $QV -kernel k_b7.bin" "$rc" \
    "note:      the kernel halts after 'B7 ok'; qmp_cmds.py then quits QEMU, so 0 is the expected exit code" "$NOTE" "$HW_NOTE"
[ "$rc" = 0 ] && grep -q '^B7 ok' .s1.txt || status=1

# 2. acceptance test 3: x2APIC CPU flag on and off
for mode in on off; do
    flag=$([ $mode = on ] && echo + || echo -)
    $QM .s2.txt "waiting for keys" "$KEYS" $QEMU $QV -cpu "qemu64,${flag}x2apic" -kernel k_b7.bin > .q2.txt 2>&1; rc=$?
    { grep '^qemu:' .q2.txt; grep -E '^apic: (IA32|local)|^B7' .s2.txt; } > x2apic_$mode.out
    rec x2apic_$mode "b7_main.cc, apic.cc" "$QEMU_VER" \
        "python3 -I qmp_cmds.py serial.txt 'waiting for keys' '$KEYS' $QEMU $QV -cpu qemu64,${flag}x2apic -kernel k_b7.bin" "$rc" \
        "note:      only the APIC lines and the result are kept; the rest matches boot.out" "$HW_NOTE"
    [ "$rc" = 0 ] && grep -q '^B7 ok' .s2.txt || status=1
done

# 3. acceptance test 1, second half: a corrupted table is rejected
qrun .s3.txt 30 k_corrupt.bin; rc=$?
strip_boot .s3.txt > corrupt.out
rec corrupt "b7_main.cc built with -DB7_CORRUPT" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_corrupt.bin" "$rc" \
    "note:      exit code 33 = the kernel reported success (the damaged MADT was rejected)" "$NOTE" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# 4. forensic evidence: the handler without EOI
$QM .s4.txt "waiting for keys" "sendkey a;sleep 1;sendkey b;sleep 1;info lapic;info irq" \
    $QEMU $QV -kernel k_noeoi.bin > .q4.txt 2>&1; rc=$?
grep -E '^irq:|^B7' .s4.txt > forensic_serial.out
cp .q4.txt forensic_monitor.out
rec forensic_serial "b7_main.cc built with -DB7_NO_EOI" "$QEMU_VER" \
    "python3 -I qmp_cmds.py serial.txt 'waiting for keys' 'sendkey a;sleep 1;sendkey b;sleep 1;info lapic;info irq' $QEMU $QV -kernel k_noeoi.bin" \
    "$rc" "note:      only the interrupt lines of the serial log are kept" "$HW_NOTE"
rec forensic_monitor "QEMU monitor: info lapic, info irq" "$QEMU_VER" "(the same run as forensic_serial)" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

rm -f k_b7.* k_corrupt.* k_noeoi.* .s?.txt .q?.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
