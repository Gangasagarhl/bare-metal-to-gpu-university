#!/usr/bin/env bash
# F3-21 run.sh: milestone B4 (paging). Steps:
#   b4 + walk       : acceptance tests 1-3, then QEMU's monitor: CR3, "info mem", a walk by hand
#   guard           : overflow of a 16 KiB kernel stack into its 4 KiB guard page
#   forensic_tlb    : a driver that unmaps without invalidating the TLB (forensic lab)
#   forensic_fixed  : the same driver with unmap() (the fix)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc \
../F3-19/gdt.cc ../F3-19/isr.S ../F3-19/interrupts.cc ../F3-20/pmm.cc paging.cc b4_main.cc"
B=../F3-18/kbuild.sh
$B k_b4 "" $SRC && $B k_ovf "-DB4_OVERFLOW" $SRC && $B k_stale "-DB4_STALE_TLB" $SRC \
    && $B k_fixed "-DB4_STALE_FIXED" $SRC || exit 1
strip_map() { grep -v -e '^  0000' -e '^firmware memory map' -e '^usable ranges' "$1"; }

# 1. acceptance tests and the monitor's view (QMP drives QEMU's human monitor)
python3 -I qmp_walk.py .serial.txt $QEMU $QBASE -kernel k_b4.bin > walk.out 2>&1; rc=$?
strip_map .serial.txt > b4.out
rec b4 "b4_main.cc, paging.cc (kbuild.sh)" "$QEMU_VER; $GXX_VER" \
    "python3 -I qmp_walk.py serial.txt $QEMU $QBASE -kernel k_b4.bin" "$rc" \
    "note:      the kernel's firmware memory map lines (as in F3-20) are left out of this file" "$HW_NOTE"
rec walk "qmp_walk.py" "$QEMU_VER; $(python3 --version)" \
    "python3 -I qmp_walk.py serial.txt $QEMU $QBASE -kernel k_b4.bin (same run)" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] && grep -q 'B4 ok' b4.out && grep -q '0x5452414e534c4154' walk.out || status=1

# 2. the guard page under the kernel stack
qrun .g.txt 30 k_ovf.bin; rc=$?
strip_map .g.txt > guard.out
rec guard "b4_main.cc built with -DB4_OVERFLOW" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_ovf.bin" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

# 3. forensic evidence and the fixed driver
qrun .f.txt 30 k_stale.bin; rc=$?
strip_map .f.txt > forensic_tlb.out
rec forensic_tlb "b4_main.cc built with -DB4_STALE_TLB" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_stale.bin" "$rc" \
    "note:      QEMU's software TLB keeps a stale translation until invlpg or a CR3 write, as a real TLB may" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
qrun .x.txt 30 k_fixed.bin; rc=$?
strip_map .x.txt > forensic_fixed.out
rec forensic_fixed "b4_main.cc built with -DB4_STALE_FIXED" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_fixed.bin" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f k_b4.* k_ovf.* k_stale.* k_fixed.* .serial.txt .g.txt .f.txt .x.txt
sed -i "s#$(pwd)/##g" ./*.out
exit $status
