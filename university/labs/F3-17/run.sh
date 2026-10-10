#!/usr/bin/env bash
# F3-17 lab (optional chapter, milestone A4 "build EDK II/OVMF and coreboot"): this build
# container has no EDK II or coreboot source tree and no internet access, so nothing is built
# from source here (see the chapter's untested box). What can be done for real: compare the
# firmware builds that the distribution's packages ship, module by module, to see what build
# options change in a binary; read the version banner of the SeaBIOS build; and compare what the
# packaged SeaBIOS and OVMF write to the firmware debug port.
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
GXX="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXXV="$(g++ --version | head -n 1)"
OVMFV="ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
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
$GXX ../F3-11/fvscan.cc -llzma -o $B/fvscan || status=1

# 1. what the firmware packages installed
{
    echo "files of the ovmf package ($(dpkg-query -W -f '${Version}' ovmf)) in /usr/share/OVMF and /usr/share/ovmf:"
    ls -l /usr/share/OVMF/ /usr/share/ovmf/ | awk 'NF >= 9 {printf "  %-28s %9s bytes %s\n", $9, $5, ($10 == "->" ? "-> " $11 : "")}'
} > packages.out 2>&1; rc=$?
rec packages "(no listing: ls)" "$OVMFV" "ls -l /usr/share/OVMF/ /usr/share/ovmf/" "$rc"

# 2. module lists of three builds, then the differences
for f in OVMF_CODE_4M.fd OVMF_CODE_4M.secboot.fd; do
    $B/fvscan /usr/share/OVMF/$f > $B/$f.txt 2>&1 || status=1
done
$B/fvscan /usr/share/ovmf/OVMF.amdsev.fd > $B/OVMF.amdsev.fd.txt 2>&1 || status=1
(cd $B && python3 ../fvdiff.py OVMF_CODE_4M.fd.txt OVMF_CODE_4M.secboot.fd.txt) > diff_secboot.out 2>&1; rc=$?
rec diff_secboot "fvdiff.py (inputs from ../F3-11/fvscan.cc)" "$GXXV; $(python3 --version); $OVMFV" \
    "$GXX ../F3-11/fvscan.cc -llzma -o fvscan; ./fvscan OVMF_CODE_4M.fd > A; ./fvscan OVMF_CODE_4M.secboot.fd > B; python3 fvdiff.py A B" "$rc"
[ "$rc" = 0 ] || status=1
(cd $B && python3 ../fvdiff.py OVMF_CODE_4M.fd.txt OVMF.amdsev.fd.txt) > diff_amdsev.out 2>&1; rc=$?
rec diff_amdsev "fvdiff.py (inputs from ../F3-11/fvscan.cc)" "$GXXV; $(python3 --version); $OVMFV" \
    "./fvscan OVMF_CODE_4M.fd > A; ./fvscan /usr/share/ovmf/OVMF.amdsev.fd > B; python3 fvdiff.py A B" "$rc"
[ "$rc" = 0 ] || status=1

# 3. the SeaBIOS build's own version and build banner (text inside the binary)
{
    echo "strings in /usr/share/seabios/bios-256k.bin that mention a version or the compiler:"
    strings -n 8 /usr/share/seabios/bios-256k.bin | grep -E '^SeaBIOS \(version|^BUILD:' | sort -u
} > seabios.out 2>&1; rc=$?
rec seabios "(no listing: strings)" "seabios package $(dpkg-query -W -f '${Version}' seabios); $(strings --version | head -n 1)" \
    "strings -n 8 /usr/share/seabios/bios-256k.bin | grep -E '^SeaBIOS \\(version|^BUILD:' | sort -u" "$rc"

# 4. is any firmware source tree or build tool present? (it is not; this is recorded, not assumed)
{
    for t in build iasl nasm git make; do
        if command -v $t > /dev/null 2>&1; then echo "command $t: $(command -v $t)"; else echo "command $t: not found"; fi
    done
    for p in uuid-dev acpica-tools; do
        v=$(dpkg-query -W -f '${Status}' $p 2>/dev/null | grep -q "install ok installed" && echo installed || echo "not installed")
        echo "package $p: $v"
    done
    for d in /usr/src/edk2 /usr/share/edk2 /usr/src/coreboot; do
        if [ -e $d ]; then echo "directory $d: exists"; else echo "directory $d: not present"; fi
    done
} > toolcheck.out 2>&1; rc=$?
rec toolcheck "(no listing: command -v, dpkg-query and test -e)" "bash $BASH_VERSION; $(dpkg-query --version | head -n 1)" \
    "command -v build iasl nasm git make; dpkg-query -W uuid-dev acpica-tools; test -e /usr/src/edk2 /usr/share/edk2 /usr/src/coreboot" "$rc"

# 5. the firmware's debug port (I/O port 0x402, QEMU's isa-debugcon): SeaBIOS as packaged, then OVMF as packaged
QEMU=qemu-system-x86_64
QARGS="-machine q35 -m 256M -display none -no-reboot -net none"
timeout 6 $QEMU $QARGS -chardev file,id=d,path=$B/seabios_dbg.txt -device isa-debugcon,iobase=0x402,chardev=d > /dev/null 2>&1
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
timeout 8 $QEMU $QARGS -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    -drive if=pflash,format=raw,file=$B/vars.fd -chardev file,id=d,path=$B/ovmf_dbg.txt -device isa-debugcon,iobase=0x402,chardev=d > /dev/null 2>&1
{
    echo "SeaBIOS (bios-256k.bin), 6 s of debug-port output: $(wc -l < $B/seabios_dbg.txt) lines; the first 12:"
    head -n 12 $B/seabios_dbg.txt | sed 's/^/  /'
    echo "OVMF_CODE_4M.fd, 8 s of debug-port output: $(wc -c < $B/ovmf_dbg.txt) bytes"
} > debugcon.out 2>&1; rc=$?
rec debugcon "(no listing: QEMU with -device isa-debugcon,iobase=0x402)" "$($QEMU --version | head -n 1); seabios package $(dpkg-query -W -f '${Version}' seabios); $OVMFV" \
    "timeout 6 $QEMU $QARGS -chardev file,id=d,path=seabios_dbg.txt -device isa-debugcon,iobase=0x402,chardev=d; timeout 8 $QEMU $QARGS -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -chardev file,id=d,path=ovmf_dbg.txt -device isa-debugcon,iobase=0x402,chardev=d" \
    "$rc (QEMU itself was stopped by timeout, status 124, as intended)" "hardware:  untested on hardware; QEMU 8.2.2 q35 machine, not a real PC"
grep -q "^SeaBIOS (version" $B/seabios_dbg.txt || status=1

# 6. forensic evidence: the plain firmware code with the variable store that holds Microsoft's keys
mkdir -p $B/esp/EFI/BOOT
clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10 \
    -c ../F3-16/sbstate.cc -o $B/sbstate.o && lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /Brepro /out:$B/esp/EFI/BOOT/BOOTX64.EFI $B/sbstate.o > /dev/null || status=1
cp /usr/share/OVMF/OVMF_VARS_4M.ms.fd $B/vars.fd
python3 ../F3-10/qemu_run.py --timeout 60 --stop-after "Press any key to enter the Boot Manager" -- $QEMU $QARGS -serial stdio \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd -drive if=pflash,format=raw,file=$B/vars.fd \
    -drive format=raw,file=fat:rw:$B/esp -device isa-debug-exit,iobase=0xf4,iosize=0x04 2>&1 | grep -v -E '^\s*$' > forensic_mixed.out; rc=${PIPESTATUS[0]}
rec forensic_mixed "../F3-16/sbstate.cc (as \\EFI\\BOOT\\BOOTX64.EFI)" "$(clang++ --version | head -n 1); $($QEMU --version | head -n 1); $OVMFV" \
    "python3 ../F3-10/qemu_run.py --timeout 60 --stop-after 'Press any key to enter the Boot Manager' -- $QEMU $QARGS -serial stdio -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.ms.fd(copy) -drive format=raw,file=fat:rw:esp -device isa-debug-exit,iobase=0xf4,iosize=0x04" \
    "$rc (33 = sbstate ran and wrote 0x10 to isa-debug-exit)" "hardware:  untested on hardware; QEMU 8.2.2 q35 machine, not a real PC"
grep -q "F3-16 sbstate: done" forensic_mixed.out || status=1

rm -rf "$B"
exit $status
