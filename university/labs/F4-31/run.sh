#!/usr/bin/env bash
# F4-31 run.sh: read the devicetree QEMU generates for its Arm virt machine (our own parser,
# cross-checked against QEMU's memory map), contrast it with PCI enumeration on a PC machine,
# boot the "hello" kernel that finds everything through the devicetree, and record the forensic
# evidence (a scanning "driver" that crashes).
set -u -o pipefail
cd "$(dirname "$0")"
. ./lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
ok() { [ "$1" = "$2" ] || { echo "F4-31 step $3: exit $1, expected $2" >&2; status=1; }; }
VIRT="qemu-system-aarch64 -M virt -cpu cortex-a53 -m 256M"

# 1. the host tool
hostbuild $B/fdtdump fdtdump.cc > fdtdump_build.out 2>&1; rc=$?
echo "build of fdtdump.cc: exit $rc (no messages above means no warnings)" >> fdtdump_build.out
rec fdtdump_build "fdtdump.cc fdt.h" "$GXX_VER" "g++ $HOSTFLAGS fdtdump.cc -o fdtdump" "$rc"
ok "$rc" 0 fdtdump_build

# 2. QEMU writes the devicetree it would give a kernel; we read it
$VIRT -machine dumpdtb=$B/virt.dtb -nographic > $B/dump.txt 2>&1
$B/fdtdump $B/virt.dtb > $B/tree.txt; rc=$?
{ head -n 31 $B/tree.txt; echo "    [...]"
  for n in 'pl011@9000000' 'apb-pclk' 'chosen'; do
      awk -v n="    $n {" '$0 == n {p=1} p {print} p && /^    };$/ {exit}' $B/tree.txt; echo "    [...]"; done
  echo "($(wc -l < $B/tree.txt) lines in all; only the first 31 lines and three whole nodes are shown)"; } > fdtdump_virt.out
rec fdtdump_virt "fdtdump.cc fdt.h" "$GXX_VER; $QEMU_VER" "$VIRT -machine dumpdtb=virt.dtb; ./fdtdump virt.dtb (trimmed: first 31 lines, nodes pl011@9000000, apb-pclk, chosen)" "$rc"
ok "$rc" 0 fdtdump_virt
$B/fdtdump --devices $B/virt.dtb > $B/dev.txt; rc=$?
grep -v 'virtio_mmio@a00[1-3]\|virtio_mmio@a000[2-9a-f]' $B/dev.txt > devices_virt.out
echo "[...] (virtio_mmio@a000200 to virtio_mmio@a003e00: 31 more rows of the same kind not shown)" >> devices_virt.out
rec devices_virt "fdtdump.cc fdt.h" "$GXX_VER" "./fdtdump --devices virt.dtb (31 identical virtio rows trimmed)" "$rc"
ok "$rc" 0 devices_virt

# 3. QEMU's own memory map of the same machine, and the cross-check
printf 'info mtree -f\nquit\n' | hmp qemu-system-aarch64 -M virt -cpu cortex-a53 -m 256M | memory_view |
    grep -E 'Root|ram\)|i/o\)|romd\)' > $B/mtree.txt
grep -E 'Root|0000000008|0000000009|flash|ram\)' $B/mtree.txt | grep -v virtio > mtree_virt.out
echo "[...] (the 32 virtio-mmio regions and the PCI windows are not shown)" >> mtree_virt.out
rec mtree_virt "QEMU monitor" "$QEMU_VER" "$VIRT -S -monitor stdio; (qemu) info mtree -f; first flat view, filtered with grep" "0"
python3 crosscheck.py $B/dev.txt $B/mtree.txt > $B/cc.txt; rc=$?
{ awk '!/virtio_mmio@a00(0[2-9a-e]|[1-3])/' $B/cc.txt | sed '/virtio_mmio@a000000/a [...] (31 more virtio_mmio rows, each matched to a "virtio-mmio" region, not shown)'; } > crosscheck.out
rec crosscheck crosscheck.py "$PY_VER" "python3 crosscheck.py devices.txt mtree.txt (31 virtio rows trimmed)" "$rc"
grep -q '0 not confirmed' crosscheck.out || { status=1; echo "crosscheck failed" >&2; }

# 4. contrast: a PC-style machine answers PCI enumeration; nobody wrote these devices down
printf 'info pci\nquit\n' | hmp qemu-system-x86_64 -M q35 -nodefaults -device virtio-rng-pci -device e1000 |
    grep -E 'Bus|class|PCI device|BAR' | sed 's/^ *//' > $B/pci.txt
mv $B/pci.txt pci_q35.out
rec pci_q35 "QEMU monitor" "$(qemu-system-x86_64 --version | head -n 1)" \
    "qemu-system-x86_64 -M q35 -nodefaults -device virtio-rng-pci -device e1000 -S -monitor stdio; (qemu) info pci" "0"

# 5. the hello kernel: console, memory, CPUs and devices all from the devicetree
kbuild $B/hello.bin $B/h hello.cc > kbuild.out 2>&1; rc=$?
{ echo "build exit code: $rc"; aarch64-linux-gnu-size $B/hello.elf | sed "s#$B/##";
  echo "image file: $(stat -c %s $B/hello.bin) bytes; first 64 bytes (the image header):";
  od -A x -t x1 -N 64 $B/hello.bin; } >> kbuild.out
rec kbuild "start.S kbase.cc kbase.h hello.cc fdt.h kernel.ld" "$XGXX_VER; $XLD_VER" \
    "aarch64-linux-gnu-g++ $KFLAGS -fpie -c <file>; ld -pie --no-dynamic-linker -T kernel.ld; objcopy -O binary" "$rc"
ok "$rc" 0 kbuild
qrun 20 -- $VIRT -nographic -semihosting -kernel $B/hello.bin > hello_virt.out; rc=$?
rec hello_virt "hello.bin" "$QEMU_VER" "$VIRT -nographic -semihosting -kernel hello.bin" "$rc" \
    "note:      exit code 0 = the kernel ended the run itself through semihosting" "$EMU_NOTE"
ok "$rc" 0 hello_virt

# 6. forensic evidence: the scanning "driver"
kbuild $B/scan.bin $B/s blindprobe.cc > $B/sb.txt 2>&1
qrun 20 -- $VIRT -nographic -semihosting -kernel $B/scan.bin > forensic_scan.out; rc=$?
rec forensic_scan "the image from the evidence pack (blindprobe.cc)" "$QEMU_VER" "$VIRT -nographic -semihosting -kernel scan.bin" "$rc" \
    "note:      exit code 3 = the kernel's exception handler stopped the run" "$EMU_NOTE"
ok "$rc" 3 forensic_scan
elr="$(sed -n 's/.*ELR_EL1=0x\([0-9a-f]*\).*/\1/p' forensic_scan.out)"
off=$(printf '0x%x' $((0x$elr - 0x40080000)))
{ echo "ELR_EL1 0x$elr - load address 0x40080000 = offset $off in the image"
  aarch64-linux-gnu-addr2line -f -C -i -e $B/scan.elf $off | sed "s#$(pwd)/##"; } > forensic_addr2line.out
rec forensic_addr2line "scan.elf (linked at 0)" "$(aarch64-linux-gnu-addr2line --version | head -n 1)" "aarch64-linux-gnu-addr2line -f -C -i -e scan.elf <ELR - load address>" "$?"

rm -rf "$B"
exit $status
