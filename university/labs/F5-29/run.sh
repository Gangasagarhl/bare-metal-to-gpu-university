#!/usr/bin/env bash
# F5-29 lab: talk IPMI to a (simulated) BMC over KCS from a UEFI program in QEMU.
# 1. record what QEMU's IPMI devices offer (their own help output);
# 2. build kcs.cc and boot it with QEMU's simulated BMC (ipmi-bmc-sim) on an ISA KCS interface;
# 3. compare the replies with the properties we gave the simulated BMC.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B/esp/EFI/BOOT"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with QEMU's simulated BMC (ipmi-bmc-sim), not a real BMC"
rec() {  # rec <name> <listing> <toolchain> <command> <exit code text> [extra line]
    {
        echo "listing:   $2"
        echo "toolchain: $3"
        echo "command:   $4"
        echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
        echo "exit code: $5"
        if [ $# -ge 6 ]; then echo "$6"; fi
    } > "$1.log"
}
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10"
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib"

# 1. the IPMI devices QEMU knows, and the properties of the simulated BMC and the KCS interface
{
    echo "\$ $QEMU -device help | grep -i ipmi"
    $QEMU -device help 2>&1 | grep -i ipmi
    for d in ipmi-bmc-sim isa-ipmi-kcs; do
        echo "\$ $QEMU -device $d,help"
        $QEMU -device $d,help 2>&1
    done
} > bmc_props.out; rc=$?
rec bmc_props "(no listing: QEMU's own help output)" "$QEMUV" \
    "$QEMU -device help | grep -i ipmi; $QEMU -device ipmi-bmc-sim,help; $QEMU -device isa-ipmi-kcs,help" "$rc"

# 2. build and boot
$UEFI_CXX -c kcs.cc -o $B/kcs.o > $B/build.txt 2>&1 &&
    $UEFI_LD /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/kcs.o >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }
BMC="-device ipmi-bmc-sim,id=bmc0,device_id=0x42,fwrev1=3,fwrev2=0x14,mfg_id=0x1234,product_id=0x5678 -device isa-ipmi-kcs,bmc=bmc0"
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 ../F3-10/qemu_run.py --timeout 90 -- $QEMU -machine q35 -m 256M -display none -serial stdio \
    -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$CODE \
    -drive if=pflash,format=raw,file=$B/vars.fd -drive format=raw,file=fat:rw:$B/esp \
    $BMC -device isa-debug-exit,iobase=0xf4,iosize=0x04 > kcs.out 2>&1; rc=$?
rec kcs "kcs.cc console.hpp efi.hpp (F3-10)" "$CLANGV; $QEMUV; $OVMFV" \
    "$UEFI_CXX -c kcs.cc && $UEFI_LD /out:BOOTX64.EFI kcs.o; python3 ../F3-10/qemu_run.py --timeout 90 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp $BMC -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
    "$rc (0 = QEMU powered the virtual machine off; 33 would mean the program reached its end without a power-down)" "$EMU"
[ "$rc" = 0 ] || status=1

# 3. checks against the properties given to ipmi-bmc-sim
{
    grep -q "firmware 3.0x14," kcs.out && r=PASS || r=FAIL
    echo "firmware revision 3.0x14 (fwrev1=3, fwrev2=0x14): $r"
    grep -q "manufacturer 0x001234, product 0x5678" kcs.out && r=PASS || r=FAIL
    echo "manufacturer 0x001234 and product 0x5678 (mfg_id, product_id): $r"
    grep -q "interface type 1 (KCS), IPMI version 2.0, BMC address 0x20, base 0x00000ca3" kcs.out && r=PASS || r=FAIL
    echo "SMBIOS type 38 describes the KCS interface at port 0xca2/0xca3 (ioport default 3234): $r"
    grep -q "SEL entries 0, free bytes 2048" kcs.out && grep -q "response: 0x2c 0x40 0x00 0x51 0x01 0x00 0xf0 0x07" kcs.out && r=PASS || r=FAIL
    echo "SEL: 0 entries before, 1 entry and 16 bytes less free space after Add SEL Entry: $r"
    grep -q "still running" kcs.out && r=FAIL || r=PASS
    echo "Chassis Control power down switched the virtual machine off (no 'still running' line, QEMU exit 0): $r"
    id=$(sed -n 's/^  device ID \(0x[0-9a-f]*\),.*/\1/p' kcs.out)
    echo "NOTE device ID: configured device_id=0x42, the BMC reported $id ($([ "$id" = 0x42 ] && echo equal || echo 'NOT equal: see F5-29, Common mistakes'))"
} > check_kcs.out
rec check_kcs "(no listing: grep and sed on kcs.out)" "$(grep --version | head -n 1)" "checks written in run.sh step 3" "0"
grep -q FAIL check_kcs.out && status=1

rm -rf "$B"
exit $status
