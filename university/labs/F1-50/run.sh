#!/usr/bin/env bash
# F1-50 run.sh: inspect QEMU's emulated NVMe controller and record the real NVMe queue
# traffic of the firmware reading our boot sector from it.
#   step qemu_pci   : the QEMU monitor's "info pci" and the NVMe part of "info qtree"
#   step nvme_trace : QEMU's own pci_nvme_* trace events (trimmed) and the boot sector's line
#   step trace_summary : Listing 2 (nvme_trace.cc) run over the complete trace
set -u -o pipefail
cd "$(dirname "$0")"
QEMU="qemu-system-x86_64"
QV="$($QEMU --version | head -n 1)"
rec() {
    {
        echo "listing:   $5"
        echo "toolchain: $2"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container; QEMU with TCG, no KVM)"
        echo "exit code: $4"
        echo "hardware:  emulated NVMe controller only; untested on a physical SSD"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
status=0
nasm -f bin boot.asm -o .boot.bin || exit 1
truncate -s 1M .nvme.img
dd if=.boot.bin of=.nvme.img conv=notrunc status=none
DEV="-drive file=.nvme.img,if=none,id=nv0,format=raw -device nvme,serial=HW205NVME,drive=nv0"

# --- step 1: monitor
C1="$QEMU -machine q35 -nodefaults -display none -serial none -no-reboot -monitor stdio $DEV (monitor: info pci, info qtree after 4 s)"
(sleep 4; echo 'info pci'; echo 'info qtree'; sleep 1; echo quit) | \
    timeout 60 $QEMU -machine q35 -nodefaults -display none -serial none -no-reboot -monitor stdio $DEV \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 -S > .mon.txt 2>&1
rc=$?
# -S keeps the CPU stopped, so the firmware has not assigned BAR addresses yet: run again without it
(sleep 4; echo 'info pci'; sleep 1; echo quit) | \
    timeout 60 $QEMU -machine q35 -nodefaults -display none -serial none -no-reboot -monitor stdio \
    -drive file=.nvme.img,if=none,id=nv0,format=raw -device nvme,serial=HW205NVME,drive=nv0 > .mon2.txt 2>&1
rc2=$?
{
    echo "== info pci (firmware still running, before our boot sector exits) =="
    sed 's/\x1b\[[0-9;]*[A-Za-z]//g' .mon2.txt | grep -v '^(qemu)' | grep -v '^QEMU .* monitor'
    echo "== info qtree, NVMe device only =="
    sed 's/\x1b\[[0-9;]*[A-Za-z]//g' .mon.txt | awk '/dev: nvme,/{f=1} f{print} f&&/bar 0:/{exit}'
    echo "== pci.ids lookups (the ID database used by lspci) =="
    awk '/^1b36 /{v=1;print;next} v&&/^\t0010 /{print;exit} /^[0-9a-f]/{v=0}' /usr/share/misc/pci.ids
    awk '/^C 01 /{c=1;print;next} c&&/^\t08 /{s=1;print;next} s&&/^\t\t02 /{print;exit} /^C /{c=0}' /usr/share/misc/pci.ids
} > qemu_pci.out
rec qemu_pci "$QV" "$C1" "$(( rc + rc2 ))" "run.sh (step qemu_pci)" \
    "note:      'quit' ends both monitor sessions; info pci prints the class as 0264 while info qtree prints Class 0108: 0x0108 = 264 in decimal"
[ $(( rc + rc2 )) -eq 0 ] || status=1

# --- step 2: trace of the firmware using the NVMe queues
C2="$QEMU -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot $DEV -device isa-debug-exit,iobase=0xf4,iosize=0x04 -debugcon file:debugcon.txt -chardev file,id=fw,path=seabios.txt -device isa-debugcon,iobase=0x402,chardev=fw -trace 'pci_nvme_*' -D trace.txt"
rm -f .trace.txt .debugcon.txt .seabios.txt
timeout 60 $QEMU -machine q35 -nodefaults -display none -serial none -monitor none -no-reboot $DEV \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 -debugcon file:.debugcon.txt \
    -chardev file,id=fw,path=.seabios.txt -device isa-debugcon,iobase=0x402,chardev=fw \
    -trace 'pci_nvme_*' -D .trace.txt
rc=$?
total=$(wc -l < .trace.txt)
{
    echo "== firmware log (I/O port 0x402), first line: the firmware that drives the NVMe queues =="
    head -n 1 .seabios.txt
    echo "== debug console (I/O port 0xe9, written by our boot sector) =="
    cat .debugcon.txt
    echo "== QEMU NVMe trace: lines 1-56 of $total =="
    sed -n '1,56p' .trace.txt
    echo "[… lines 57-$(( total - 13 )) trimmed: the same pattern repeated: Identify Namespace commands for namespace ids 1, 2, 3 … 256 and their completions]"
    echo "== QEMU NVMe trace: last 13 lines =="
    tail -n 13 .trace.txt
} > nvme_trace.out
rec nvme_trace "$QV; $(nasm -v)" "$C2" "$rc" "run.sh (step nvme_trace) and boot.asm" \
    "note:      exit code 33 is expected: the boot sector wrote 0x10 to the isa-debug-exit port and QEMU exits with (0x10 << 1) | 1"
[ "$rc" -eq 33 ] || status=1

# --- step 3: Listing 2 over the complete trace
B="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined nvme_trace.cc -o nvme_trace"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined nvme_trace.cc -o .bin_trace || exit 1
./.bin_trace < .trace.txt > trace_summary.out 2>&1
rc=$?
rec trace_summary "$(g++ --version | head -n 1)" "$B && ./nvme_trace < trace.txt (the complete trace of step nvme_trace)" "$rc" "nvme_trace.cc"
[ "$rc" -eq 0 ] || status=1
rm -f .boot.bin .nvme.img .mon.txt .mon2.txt .trace.txt .debugcon.txt .seabios.txt .bin_trace
exit $status
