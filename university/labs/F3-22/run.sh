#!/usr/bin/env bash
# F3-22 run.sh: milestone B5 (kernel heap). Steps:
#   b5        : containers, every operator new/delete form, 1,000,000-allocation stress test
#   uaf       : a deliberate use-after-free, caught by the poison check (exit 35 as designed)
#   forensic  : the forensic "name table" kernel
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc \
../F3-19/gdt.cc ../F3-19/isr.S ../F3-19/interrupts.cc ../F3-20/pmm.cc ../F3-21/paging.cc kheap.cc new.cc b5_main.cc"
B=../F3-18/kbuild.sh
$B k_b5 "" $SRC && $B k_uaf "-DB5_UAF" $SRC && $B k_for "-DB5_FORENSIC" $SRC || exit 1
strip_map() { grep -v -e '^  0000' -e '^firmware memory map' -e '^usable ranges' -e '^gdt:' -e '^idt:' -e '^paging:' "$1"; }
NOTE="note:      boot lines already shown in F3-19 to F3-21 (memory map, gdt, idt, paging) are left out"

qrun .a.txt 120 k_b5.bin; rc=$?
strip_map .a.txt > b5.out
rec b5 "b5_main.cc, kheap.cc, heap.h, new.cc, containers.h" "$QEMU_VER; $GXX_VER" \
    "$QEMU $QBASE -serial stdio -kernel k_b5.bin" "$rc" "$NOTE" "$HW_NOTE"
[ "$rc" = 33 ] && grep -q 'B5 ok' b5.out || status=1

qrun .b.txt 30 k_uaf.bin; rc=$?
strip_map .b.txt > uaf.out
rec uaf "b5_main.cc built with -DB5_UAF" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_uaf.bin" "$rc" \
    "note:      exit code 35 is expected: the poison check reports and panics" "$NOTE" "$HW_NOTE"
[ "$rc" = 35 ] && grep -q 'kmalloc-64' uaf.out || status=1

qrun .c.txt 30 k_for.bin; rc=$?
strip_map .c.txt > forensic.out
rec forensic "b5_main.cc built with -DB5_FORENSIC" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_for.bin" "$rc" "$NOTE" "$HW_NOTE"
[ "$rc" = 35 ] || status=1

rm -f k_b5.* k_uaf.* k_for.* .a.txt .b.txt .c.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
