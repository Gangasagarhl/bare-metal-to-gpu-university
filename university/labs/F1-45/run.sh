#!/usr/bin/env bash
# F1-45: (1) QEMU's own PCI listing of a q35 PC with an e1000 NIC, an xHCI USB
# controller, and a PCIe root port with a virtio RNG behind it; (2) lspci on the
# build machine, to compare with cfg_read.out (Listing 1's run).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <listing> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
    [ "$5" = 0 ] || status=1
}
QX="$(qemu-system-x86_64 --version | head -n 1)"
DEV="-netdev user,id=n0 -device e1000,netdev=n0 -device qemu-xhci -device pcie-root-port,id=rp1,chassis=1,bus=pcie.0 -device virtio-rng-pci,bus=rp1"
python3 -I hmp.py 3 "info pci" qemu-system-x86_64 -machine q35 -display none -serial null -nographic \
    -no-reboot $DEV > qemu_info_pci.out 2>&1; rc=$?
rec qemu_info_pci hmp.py "$QX" \
    "python3 -I hmp.py 3 \"info pci\" qemu-system-x86_64 -machine q35 -display none -serial null -nographic -no-reboot $DEV" \
    "$rc" "note:      the SeaBIOS firmware ran for about 3 s first, so the BAR addresses are assigned"
lspci -n > lspci.out 2>&1; rc=$?
rec lspci "(none: lspci is a system tool)" "$(lspci --version 2>&1 | head -n 1)" "lspci -n" "$rc" \
    "note:      the build container's own virtual machine; your machine will list other devices"
lspci > lspci_names.out 2>/dev/null; rc=$?
lspci -v -s 00:08.0 >> lspci_names.out 2>/dev/null || rc=1
rec lspci_names "(none: lspci is a system tool)" "$(lspci --version 2>&1 | head -n 1)" \
    "lspci; lspci -v -s 00:08.0 (stderr discarded: a harmless libkmod warning)" "$rc" \
    "note:      names come from the pciutils ID database installed on the build machine"
exit $status
