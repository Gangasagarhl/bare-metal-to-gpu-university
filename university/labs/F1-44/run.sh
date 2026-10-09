#!/usr/bin/env bash
# F1-44: an emulated IOMMU. QEMU 8.2.2's q35 PC with an Intel VT-d IOMMU model:
# 'info mtree -f' shows that every PCI device gets its own DMA address space
# ("vtd-root"), separate from the CPU's. Only firmware runs (no OS), so DMA
# remapping is still off ("vtd-nodmar") and the MSI window is the IOMMU's
# interrupt-remapping region ("vtd-ir").
set -u -o pipefail
cd "$(dirname "$0")"
status=0
QX="$(qemu-system-x86_64 --version | head -n 1)"
C="python3 -I hmp.py 2 \"info mtree -f\" qemu-system-x86_64 -machine q35,kernel-irqchip=split -device intel-iommu,intremap=on -display none -serial null -nographic -no-reboot"
python3 -I hmp.py 2 "info mtree -f" qemu-system-x86_64 -machine q35,kernel-irqchip=split \
    -device intel-iommu,intremap=on -display none -serial null -nographic -no-reboot \
    > .m.txt 2>&1; rc=$?
# keep: the CPU's view header and the device view (the FlatView that contains vtd-root)
awk '/^FlatView/{blk=""} {blk=blk $0 "\n"} /^$/{ if (blk ~ /AS "memory"|vtd-root/) printf "%s", blk; blk="" }
     END{ if (blk ~ /AS "memory"|vtd-root/) printf "%s", blk }' .m.txt \
    | grep -E '^FlatView|AS "|Root memory region|pc.ram @0000000000100000|intel_iommu|vtd-ir|apic-msi|e1000e-mmio' \
    > qemu_iommu.out
rm -f .m.txt
[ -s qemu_iommu.out ] || rc=1
{
    echo "listing:   hmp.py"
    echo "toolchain: $QX"
    echo "command:   $C"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
    echo "exit code: $rc"
    echo "note:      output filtered to the CPU's view and the devices' view; firmware only"
} > qemu_iommu.log
[ "$rc" = 0 ] || status=1
exit $status
