#!/usr/bin/env bash
# F4-08 run.sh: milestone C7 in QEMU (q35).
#   msicheck : host program, MSI/MSI-X constants checked against <linux/pci_regs.h> (run_lab.sh)
#   build    : the F4-08 kernel (default) and the forensic variant
#   msi      : edu (MSI) + qemu-xhci (MSI-X), no IOMMU: identity DMA, the stray write lands
#   iommu    : the same with intel-iommu: translated DMA, the stray write becomes a DMAR fault
#   forensic : intel-iommu, the receive path built with -DF408_FREE_TOO_EARLY
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-01/lablib.sh
status=0
SRC="../F4-01/boot.S ../F4-01/kbase.cc ../F4-01/div64.cc ../F4-02/pci.cc ../F4-02/acpi.cc \
isr256.S intr.cc msi.cc vtd.cc dma.cc edu.cc xhci_core.cc f408_main.cc"
DEVS="-device edu -device qemu-xhci"
IOMMU="-device intel-iommu"

kbuild k408.elf $SRC > .kb.txt 2>&1; rc=$?
KEXTRA="-DF408_FREE_TOO_EARLY" kbuild k408f.elf $SRC >> .kb.txt 2>&1 || rc=1
{ cat .kb.txt; size -A k408.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k408.elf *.o (and again with -DF408_FREE_TOO_EARLY)" "$rc"
[ "$rc" = 0 ] || status=1

qboot msi.out 60 q35 k408.elf $DEVS; rc=$?
rec msi "f408_main.cc (kernel k408.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k408.elf $DEVS" "$rc" \
    "note:      exit code 33 = pass (isa-debug-exit 0x10)" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

qboot iommu.out 60 q35 k408.elf $IOMMU $DEVS; rc=$?
rec iommu "f408_main.cc (kernel k408.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k408.elf $IOMMU $DEVS" "$rc" \
    "note:      exit code 33 = pass; lines not starting with a kernel prefix were printed by QEMU itself on stderr" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

qboot forensic.out 60 q35 k408f.elf $IOMMU $DEVS; rc=$?
rec forensic "f408_main.cc built with -DF408_FREE_TOO_EARLY (kernel k408f.elf)" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k408f.elf $IOMMU $DEVS" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

rm -f k408.elf k408f.elf .kb.txt
exit $status
