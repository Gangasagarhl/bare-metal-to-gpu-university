#!/usr/bin/env bash
# F1-43: look at real (emulated) interrupt controllers in QEMU 8.2.2:
# the x86 q35 PC (I/O APIC and two 8259 PICs), the RISC-V virt board (PLIC,
# ACLINT) and the Arm virt board (GICv3). Only firmware runs (no OS).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
rec() {  # rec <name> <toolchain> <command> <exit code> [extra line]
    {
        echo "listing:   hmp.py"
        echo "toolchain: $2"
        echo "command:   $3"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $4"
        if [ $# -ge 5 ]; then echo "$5"; fi
    } > "$1.log"
    [ "$4" = 0 ] || status=1
}
QX="$(qemu-system-x86_64 --version | head -n 1)"
C="python3 -I hmp.py 3 \"info pic;info irq\" qemu-system-x86_64 -machine q35 -display none -serial null -nographic -no-reboot"
python3 -I hmp.py 3 "info pic;info irq" qemu-system-x86_64 -machine q35 -display none -serial null \
    -nographic -no-reboot > qemu_x86_pic.out 2>&1; rc=$?
rec qemu_x86_pic "$QX" "$C" "$rc" \
    "note:      only the SeaBIOS firmware ran for about 3 s; the 'info irq' counts change from run to run"
QR="$(qemu-system-riscv64 --version | head -n 1)"
python3 -I hmp.py 1 "info mtree" qemu-system-riscv64 -machine virt -display none -serial null \
    -nographic -bios none > .r.txt 2>&1; rc=$?
awk '/^memory-region/{s=0} /^address-space: memory$/{s=1;print;next} /^address-space/{s=0} s' .r.txt \
    | grep -E '^address-space|plic|aclint|serial|virtio-mmio' | awk '!seen[$0]++' | head -n 8 > qemu_riscv_mtree.out
rec qemu_riscv_mtree "$QR" \
    "python3 -I hmp.py 1 \"info mtree\" qemu-system-riscv64 -machine virt -display none -serial null -nographic -bios none" \
    "$rc" "note:      filtered: the 'memory' address space, lines naming plic, aclint, serial and the first virtio-mmio slots"
QA="$(qemu-system-aarch64 --version | head -n 1)"
python3 -I hmp.py 1 "info mtree" qemu-system-aarch64 -machine virt,gic-version=3 -cpu cortex-a57 \
    -display none -serial null -nographic > .a.txt 2>&1; rc=$?
awk '/^memory-region/{s=0} /^address-space: memory$/{s=1;print;next} /^address-space/{s=0} s' .a.txt \
    | grep -E '^address-space|gic|pl011' | awk '!seen[$0]++' > qemu_arm_mtree.out
rec qemu_arm_mtree "$QA" \
    "python3 -I hmp.py 1 \"info mtree\" qemu-system-aarch64 -machine virt,gic-version=3 -cpu cortex-a57 -display none -serial null -nographic" \
    "$rc" "note:      filtered: the 'memory' address space, lines naming the GIC parts and the PL011 UART"
rm -f .r.txt .a.txt
# the build machine's own controllers, as Linux names them (columns: per-CPU counts, controller, pin/type, device)
{ head -n 1 /proc/interrupts; grep -E 'IO-APIC|PCI-MSI' /proc/interrupts | head -n 6; grep -E '^ *(LOC|NMI|SPU):' /proc/interrupts; } \
    | sed 's/  */ /g' > proc_interrupts.out; rc=$?
{
    echo "listing:   (none: reads a Linux kernel file)"
    echo "toolchain: Linux $(uname -r)"
    echo "command:   head -n 1 /proc/interrupts; grep -E 'IO-APIC|PCI-MSI' /proc/interrupts | head -n 6; grep LOC/NMI/SPU (spaces squeezed)"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container), $(nproc) CPUs visible"
    echo "exit code: $rc"
    echo "note:      the counts are totals since the machine started and change all the time"
} > proc_interrupts.log
exit $status
