#!/usr/bin/env bash
# F4-33 run.sh: an AArch64 UEFI application (ebbrprobe.efi) run by the EDK II firmware for QEMU's
# virt machine, once with ACPI tables (acpi=on) and once with a devicetree only (acpi=off);
# then the forensic evidence (a loader whose devicetree lookup fails).
set -u -o pipefail
cd "$(dirname "$0")"
. ../F4-31/lablib.sh
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/esp"
ok() { [ "$1" = "$2" ] || { echo "F4-33 step $3: exit $1, expected $2" >&2; status=1; }; }
FW=/usr/share/qemu-efi-aarch64/QEMU_EFI.fd
FW_VER="firmware: $FW from the Ubuntu package qemu-efi-aarch64 $(dpkg-query -W -f '${Version}' qemu-efi-aarch64)"
UC="clang++ --target=aarch64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -mgeneral-regs-only -O2 -Wall -Wextra -Wpedantic -Werror"
UL="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:arm64"
CLANG_VER="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"

uefi_build() {  # uefi_build SOURCE OUT.efi
    $UC -c "$1" -o $B/app.o && $UC -c ../F4-31/kbase.cc -o $B/kbase.o && $UL /out:"$2" $B/app.o $B/kbase.o
}

# 1. build the PE32+ application for AArch64 and show its header
{ uefi_build ebbrprobe.cc $B/esp/ebbrprobe.efi &&
  llvm-readobj --file-headers $B/esp/ebbrprobe.efi | sed "s#$B/esp/##" |
      grep -E 'File:|Format:|Arch:|Machine:|Magic:|Subsystem:|AddressOfEntryPoint:'; } > build.out 2>&1; rc=$?
rec build "ebbrprobe.cc efi_a64.h ../F4-31/fdt.h ../F4-31/kbase.cc" "$CLANG_VER" \
    "$UC -c ebbrprobe.cc; $UC -c ../F4-31/kbase.cc; $UL /out:ebbrprobe.efi ebbrprobe.o kbase.o; llvm-readobj --file-headers" "$rc"
ok "$rc" 0 build

# 2. run it twice; startup.nsh makes the UEFI Shell start it and then power the machine off
printf 'fs0:\r\nebbrprobe.efi\r\nreset -s\r\n' > $B/esp/startup.nsh
for mode in on off; do
    name=probe_acpi; [ "$mode" = off ] && name=probe_dt
    Q="qemu-system-aarch64 -M virt,acpi=$mode -cpu cortex-a57 -m 512M -nographic -net none -bios $FW -drive format=raw,file=fat:rw:$B/esp"
    timeout 90 $Q < /dev/null 2>&1 | python3 uefi_clean.py > $name.out; rc=${PIPESTATUS[0]}
    sed -i "s#$B/##g" $name.out
    rec $name ebbrprobe.efi "$QEMU_VER; $FW_VER" "${Q//$B\//} (startup.nsh: fs0:, ebbrprobe.efi, reset -s); output through uefi_clean.py" "$rc" \
        "note:      exit code 0 = the shell's reset -s powered the virtual machine off" "$EMU_NOTE"
    ok "$rc" 0 $name
    grep -q "ebbrprobe: done" $name.out || status=1
done

# 3. forensic evidence: the loader from the evidence pack (one constant differs, see the key)
sed 's/0xf19c, 0x41a5/0xf19c, 0x41a6/' ebbrprobe.cc > .forensic_loader.cc
mkdir -p $B/esp2
{ uefi_build .forensic_loader.cc $B/esp2/ebbrprobe.efi; } > $B/fb.txt 2>&1
cp $B/esp/startup.nsh $B/esp2/
Q="qemu-system-aarch64 -M virt,acpi=off -cpu cortex-a57 -m 512M -nographic -net none -bios $FW -drive format=raw,file=fat:rw:$B/esp2"
timeout 90 $Q < /dev/null 2>&1 | python3 uefi_clean.py | sed -n '/ebbrprobe: UEFI/,/ebbrprobe: done/p' > forensic_loader.out; rc=${PIPESTATUS[0]}
rec forensic_loader "the loader build from the evidence pack (ebbrprobe.efi)" "$QEMU_VER; $FW_VER" "${Q//$B\//}" "$rc" "$EMU_NOTE"
ok "$rc" 0 forensic_loader

rm -rf "$B" .forensic_loader.cc
exit $status
