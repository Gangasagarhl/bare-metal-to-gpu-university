#!/usr/bin/env bash
# F4-19 run.sh: a tour of the hardware catalog with real machines.
#   machines       the device records of F4-14's three machines (build VM, q35, AArch64 virt)
#   catalog        catalog.cc puts each device into a catalog row (or says "unclassified")
#   qemu_devices   the emulator's own list of device types, per category
#   forensic       catalog.cc built with -DF419_BASE_ONLY (matches PCI by base class only)
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-14/dr401lib.sh
status=0

python3 mk_machines.py ../F4-14 > machines.out 2>&1; rc=$?
rec machines "mk_machines.py" "$PY_VER" "python3 mk_machines.py ../F4-14 (inventory.out, devreport.out, fdt_virt.out)" "$rc"
[ "$rc" = 0 ] || status=1

hostbuild .catalog catalog.cc && ./.catalog < machines.out > catalog.out 2>&1; rc=$?
rec catalog "catalog.cc" "$GXX_VER" "g++ $HOSTFLAGS catalog.cc -o catalog; ./catalog < machines.out" "$rc"
[ "$rc" = 0 ] || status=1

$QEMU -device help 2>&1 | python3 qemu_devices.py > qemu_devices.out 2>&1; rc=$?
rec qemu_devices "qemu_devices.py" "$QEMU_VER; $PY_VER" "qemu-system-x86_64 -device help | python3 qemu_devices.py" "$rc"
[ "$rc" = 0 ] || status=1

g++ $HOSTFLAGS -DF419_BASE_ONLY catalog.cc -o .catalog_f && ./.catalog_f < machines.out > forensic.out 2>&1; rc=$?
rec forensic "catalog.cc built with -DF419_BASE_ONLY" "$GXX_VER" "g++ $HOSTFLAGS -DF419_BASE_ONLY catalog.cc -o catalog_f; ./catalog_f < machines.out" "$rc"
[ "$rc" = 0 ] || status=1

rm -f .catalog .catalog_f
exit $status
