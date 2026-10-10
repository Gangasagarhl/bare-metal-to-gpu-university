#!/usr/bin/env bash
# F4-14 run.sh: stage (a), "identify", on two more machines besides the build VM (whose report
# comes from inventory.cpp, acpi_ids.cpp and cpuid_id.cpp, run by run_lab.sh):
#   build           the DR401 lab kernel with the device-report main (f414_main.cc)
#   devreport       boot it on QEMU's q35 PC with seven extra PCI devices; DSDT bytes saved
#   qmp_compare     compare the kernel's report with QEMU's own list (QMP query-pci)
#   names           put pci.ids names on the kernel's report
#   acpi_qemu       acpi_ids.cpp on the DSDT the kernel dumped
#   fdt_virt        an Arm board: QEMU's AArch64 "virt" machine devicetree, compatible strings
#   forensic        the kernel built with F414_FUNC0_ONLY, and its comparison (expected to differ)
set -u -o pipefail
cd "$(dirname "$0")"
. ./dr401lib.sh
status=0
SRC="boot.S k4.cc pci4.cc acpi4.cc f414_main.cc"
DEV="-netdev user,id=n0 -device e1000,netdev=n0 -device edu -device nvme,serial=dr401,drive=nv \
-drive if=none,id=nv,file=/dev/null,format=raw -device qemu-xhci -device intel-hda \
-audiodev none,id=snd0 -device hda-output,audiodev=snd0 -device virtio-rng-pci"

kbuild k414.elf $SRC > .kb.txt 2>&1; rc=$?
{ cat .kb.txt; size -A k414.elf | grep -E '^(section|\.text|\.rodata|\.data|\.bss|Total)'; } > build.out
rec build "$SRC kernel.ld" "$GXX_VER; $LD_VER" \
    "g++ $KFLAGS -c <each file>; ld -m elf_i386 -nostdlib -static -T kernel.ld -o k414.elf *.o" "$rc"
[ "$rc" = 0 ] || status=1

qboot .report.txt 60 k414.elf $DEV; rc=$?
python3 - .report.txt .qemu_dsdt.bin > devreport.out <<'PY'
import sys
data = bytearray(); n = 0
for line in open(sys.argv[1], encoding="utf-8"):
    if line.startswith("dsdt "):
        data += bytes(int(x, 16) for x in line.split(":", 1)[1].split()); n += 1
        continue
    if n and line.startswith("devreport: done"):
        print("[... %d lines of DSDT hex dump trimmed by run.sh: %d bytes saved for acpi_ids ...]" % (n, len(data)))
    print(line, end="")
open(sys.argv[2], "wb").write(data)
PY
rec devreport "f414_main.cc (kernel k414.elf)" "$QEMU_VER (SeaBIOS from the QEMU package)" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k414.elf $DEV" "$rc" \
    "note:      exit code 33 = pass: the kernel wrote 0x10 to isa-debug-exit, QEMU exits with (0x10 << 1) | 1" "$HW_NOTE"
[ "$rc" = 33 ] || status=1

python3 compare.py .report.txt $DEV > qmp_compare.out 2>&1; rc=$?
rec qmp_compare "compare.py" "$PY_VER; $QEMU_VER" "python3 compare.py devreport $DEV" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

python3 name_report.py .report.txt > names.out 2>&1; rc=$?
rec names "name_report.py" "$PY_VER; pci.ids $(grep -m1 'Version:' /usr/share/misc/pci.ids | awk '{print $3}')" \
    "python3 name_report.py devreport /usr/share/misc/pci.ids" "$rc"
[ "$rc" = 0 ] || status=1

hostbuild .acpi_ids acpi_ids.cpp && ./.acpi_ids .qemu_dsdt.bin > acpi_qemu.out 2>&1; rc=$?
rec acpi_qemu "acpi_ids.cpp" "$GXX_VER" "g++ $HOSTFLAGS acpi_ids.cpp -o acpi_ids; ./acpi_ids qemu_dsdt.bin" "$rc" "$HW_NOTE"
[ "$rc" = 0 ] || status=1

timeout 30 qemu-system-aarch64 -machine virt,dumpdtb=.virt.dtb -cpu cortex-a57 -m 256M -nographic -nodefaults \
    > .dump.txt 2>&1 && hostbuild .fdt_compat fdt_compat.cc && { cat .dump.txt | sed 's#/[^ ]*/\.virt\.dtb#virt.dtb#';
    ./.fdt_compat .virt.dtb; } > fdt_virt.out 2>&1; rc=$?
rec fdt_virt "fdt_compat.cc" "$GXX_VER; $QA64_VER" \
    "qemu-system-aarch64 -machine virt,dumpdtb=virt.dtb -cpu cortex-a57 -m 256M -nographic -nodefaults; g++ $HOSTFLAGS fdt_compat.cc -o fdt_compat; ./fdt_compat virt.dtb" \
    "$rc" "hardware:  untested on a real Arm board; the devicetree is the one QEMU 8.2.2 generates for its virt machine"
[ "$rc" = 0 ] || status=1

KEXTRA="-DF414_FUNC0_ONLY" kbuild k414f.elf $SRC > .kbf.txt 2>&1 || status=1
qboot .forensic.txt 60 k414f.elf $DEV; rc=$?
grep -v '^dsdt ' .forensic.txt > forensic.out
rec forensic "f414_main.cc and pci4.cc built with -DF414_FUNC0_ONLY" "$QEMU_VER" \
    "$QEMU -machine q35 $QCOMMON -serial stdio -kernel k414f.elf $DEV" "$rc" "$HW_NOTE"
[ "$rc" = 33 ] || status=1
python3 compare.py .forensic.txt $DEV > forensic_compare.out 2>&1; rc=$?
rec forensic_compare "compare.py" "$PY_VER; $QEMU_VER" "python3 compare.py forensic $DEV" "$rc" \
    "note:      exit code 1 is the expected result here: the forensic kernel's list differs from QEMU's" "$HW_NOTE"
[ "$rc" = 1 ] || status=1

rm -f k414.elf k414f.elf .kb.txt .kbf.txt .report.txt .forensic.txt .qemu_dsdt.bin .virt.dtb .dump.txt .acpi_ids .fdt_compat
exit $status
