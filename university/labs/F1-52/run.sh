#!/usr/bin/env bash
# F1-52 run.sh: inspect QEMU's emulated NICs and record a real descriptor-ring trace.
#   step qemu_nics : the QEMU monitor's "info pci" for an e1000, an e1000e and a virtio-net
#                    NIC, with the names the pci.ids database gives their IDs
#   step ring_trace: QEMU's own e1000e trace events while the iPXE boot ROM (a real driver)
#                    sets up its rings and does DHCP, next to tshark's view of the same frames
set -u -o pipefail
cd "$(dirname "$0")"
QEMU="qemu-system-x86_64"
QV="$($QEMU --version | head -n 1)"
TV="$(tshark --version 2>/dev/null | head -n 1)"
rec() {
    {
        echo "listing:   run.sh (step $1)"
        echo "toolchain: $2"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container; QEMU with TCG, no KVM)"
        echo "exit code: $4"
        echo "hardware:  emulated NICs only (QEMU device models); untested on a physical NIC"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
}
status=0
NICS="-netdev user,id=n0 -device e1000,netdev=n0 -netdev user,id=n1 -device e1000e,netdev=n1 -netdev user,id=n2 -device virtio-net-pci,netdev=n2"

# --- step 1
C1="$QEMU -machine q35 -nodefaults -display none -serial none -no-reboot -boot order=c -monitor stdio $NICS (monitor: info pci after 4 s)"
(sleep 4; echo 'info pci'; sleep 1; echo quit) | \
    timeout 60 $QEMU -machine q35 -nodefaults -display none -serial none -no-reboot -boot order=c -monitor stdio $NICS > .mon.txt 2>&1
rc=$?
{
    sed 's/\x1b\[[0-9;]*[A-Za-z]//g' .mon.txt | grep -v '^(qemu)' | grep -v '^QEMU .* monitor' | \
        awk '/^  Bus/{blk=$0; keep=0; next} {blk=blk "\n" $0} /Ethernet controller/{keep=1} /id ""/{if(keep) print blk; keep=0}'
    echo "== pci.ids lookups (the ID database used by lspci) =="
    for id in 8086:100e 8086:10d3 1af4:1000; do
        v=${id%%:*}; d=${id##*:}
        awk -v v="$v" -v d="$d" '$1==v && /^[0-9a-f]/{f=1; vn=$0; next} f && /^\t[0-9a-f]/ && $1==d {print vn " /" $0; exit} /^[0-9a-f]/{f=0}' /usr/share/misc/pci.ids
    done
} > qemu_nics.out
rec qemu_nics "$QV" "$C1" "$rc"
[ "$rc" -eq 0 ] || status=1

# --- step 2
rm -f .ring.txt .ring.pcap
EV="-trace e1000e_core_write -trace e1000e_rx_set_rdt -trace e1000e_tx_descr -trace e1000e_rx_descr -trace e1000e_rx_written_to_guest -trace e1000e_irq_set -trace e1000e_link_status"
C2="$QEMU -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot -boot order=n -netdev user,id=n0 -device e1000e,netdev=n0,mac=52:54:00:12:34:56 -object filter-dump,id=f1,netdev=n0,file=ring.pcap $EV -D trace.txt  (stopped after 20 s by timeout)"
timeout 20 $QEMU -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot -boot order=n \
    -netdev user,id=n0 -device e1000e,netdev=n0,mac=52:54:00:12:34:56 \
    -object filter-dump,id=f1,netdev=n0,file=.ring.pcap $EV -D .ring.txt > /dev/null 2>&1
qrc=$?
{
    echo "== QEMU e1000e trace, from the first register write to the end of the first DHCP exchange =="
    # stop after the fifth transmit descriptor: the boot ROM then shuts the NIC down
    awk '{print} /e1000e_tx_descr/{n++} n==5 && /e1000e_tx_descr/{exit}' .ring.txt
    echo "[… the rest of the trace (the ROM resetting the NIC and giving up) is not shown …]"
    echo "== tshark -r ring.pcap (the same run's frames, as captured by QEMU) =="
    tshark -r .ring.pcap 2>&1 | grep -v '^Running as user "root"'
} > ring_trace.out
rec ring_trace "$QV; $TV" "$C2" "$qrc" \
    "note:      exit code 124 is expected: QEMU was stopped by the 20 s time limit after the boot ROM gave up"
[ "$qrc" -eq 124 ] || status=1
rm -f .mon.txt .ring.txt .ring.pcap
exit $status
