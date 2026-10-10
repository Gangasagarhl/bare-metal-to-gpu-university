#!/usr/bin/env bash
# F3-20 run.sh: milestone B3 (physical memory manager). Boots the test kernel with 128 MiB,
# 1 GiB and 4 GiB of RAM (acceptance tests 2 and 3), then the forensic "leaking" driver.
set -u -o pipefail
cd "$(dirname "$0")"
. ../F3-18/oslab.sh
status=0
SRC="../F3-18/boot.S ../F3-18/cxxrt.cc ../F3-18/serial.cc ../F3-18/kprint.cc ../F3-18/panic.cc pmm.cc b3_main.cc"
B=../F3-18/kbuild.sh
$B k_b3 "" $SRC && $B k_leak "-DB3_LEAK" $SRC || exit 1
for m in 128M 1G 4G; do
    name="mem_$m"
    qrun "$name.out" 120 k_b3.bin -m "$m"; rc=$?
    rec "$name" "b3_main.cc, pmm.cc, buddy.h, memmap.h (kbuild.sh)" "$QEMU_VER; $GXX_VER" \
        "$QEMU $QBASE -m $m -serial stdio -kernel k_b3.bin" "$rc" \
        "note:      the later -m option overrides the -m 128M in the base options" "$HW_NOTE"
    [ "$rc" = 33 ] && grep -q 'B3 ok' "$name.out" || status=1
done
qrun forensic_leak.out 60 k_leak.bin; rc=$?
rec forensic_leak "b3_main.cc built with -DB3_LEAK" "$QEMU_VER" "$QEMU $QBASE -serial stdio -kernel k_leak.bin" "$rc" \
    "note:      exit code 35 is expected: the driver's allocation fails and the kernel panics" "$HW_NOTE"
[ "$rc" = 35 ] || status=1
rm -f k_b3.* k_leak.*
sed -i "s#$(pwd)/##g" ./*.out
exit $status
