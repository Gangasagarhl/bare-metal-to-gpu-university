#!/usr/bin/env bash
# F3-12 lab: read the tables the firmware gives the OS (UEFI configuration table, ACPI, SMBIOS)
# from a UEFI application, compare them with QEMU's configuration, and record two timed boots
# for the forensic lab (healthy, and with a firmware boot-menu wait configured).
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
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine with the distribution's OVMF, not a real PC"
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

# 1. build tables.efi and install it at the removable-media path, so OVMF starts it by itself
$UEFI_CXX -c tables.cc -o $B/tables.o > $B/build.txt 2>&1 &&
    $UEFI_LD /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/tables.o >> $B/build.txt 2>&1 || { cat $B/build.txt; status=1; }

# 2. boot with two CPUs and our own SMBIOS strings
QARGS="-machine q35 -smp 2 -m 256M -display none -serial stdio -no-reboot -net none"
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 ../F3-10/qemu_run.py --timeout 60 -- $QEMU $QARGS -smbios type=1,manufacturer=OS301-Lab,product=Tables-Demo \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
    -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 > tables.out 2>&1; rc=$?
rec tables "tables.cc console.hpp efi.hpp" "$CLANGV; $QEMUV; $OVMFV" \
    "$UEFI_CXX -c tables.cc && $UEFI_LD /out:BOOTX64.EFI tables.o; python3 ../F3-10/qemu_run.py --timeout 60 -- $QEMU $QARGS -smbios type=1,manufacturer=OS301-Lab,product=Tables-Demo -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
    "$rc (33 = the application wrote 0x10 to isa-debug-exit after printing everything)" "$EMU"
[ "$rc" = 33 ] || status=1

# 3. compare with the configuration we gave QEMU
{
    n=$(grep -c "type 0 processor local APIC" tables.out)
    echo "MADT processor entries: $n (QEMU was started with -smp 2): $([ "$n" = 2 ] && echo PASS || echo FAIL)"
    grep -q "manufacturer 'OS301-Lab' product 'Tables-Demo'" tables.out && r=PASS || r=FAIL
    echo "SMBIOS type 1 strings equal the -smbios option: $r"
    bad=$(grep -c "checksum BAD" tables.out)
    echo "tables with a bad checksum: $bad: $([ "$bad" = 0 ] && echo PASS || echo FAIL)"
    grep -q "signature 'RSD PTR ', revision 2" tables.out && r=PASS || r=FAIL
    echo "RSDP signature 'RSD PTR ' and revision 2: $r"
} > check_tables.out
rec check_tables "(no listing: grep on tables.out)" "$(grep --version | head -n 1)" "grep checks written in run.sh step 3" "0"
grep -q FAIL check_tables.out && status=1

# 4. forensic evidence: boots of the same disk with host time stamps on every console line,
#    three times each. Configuration A: as above. Configuration B: the same, plus a firmware
#    boot-menu wait configured in QEMU (-boot menu=on,splash-time=3000).
for run in healthy slow; do
    extra=""
    [ "$run" = slow ] && extra="-boot menu=on,splash-time=3000"
    : > timeline_$run.out
    for i in 1 2 3; do
        cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
        echo "== boot $i of 3" >> timeline_$run.out
        python3 ../F3-10/qemu_run.py --timeout 60 --stamp -- $QEMU $QARGS $extra \
            -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
            -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 2>&1 \
            | grep -E "BdsDxe: starting|time-stamp|tables: done" >> timeline_$run.out; rc=${PIPESTATUS[0]}
        [ "$rc" = 33 ] || status=1
    done
    rec timeline_$run "tables.cc (BOOTX64.EFI)" "$QEMUV; $OVMFV" \
        "three times: python3 ../F3-10/qemu_run.py --timeout 60 --stamp -- $QEMU $QARGS $extra -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 | grep -E 'BdsDxe: starting|time-stamp|tables: done'" \
        "$rc (status of the third boot; all three must be 33)" "$EMU; the first column is host seconds since QEMU started (measured in this container, TCG emulation, shared with other work)"
done

rm -rf "$B"
exit $status
