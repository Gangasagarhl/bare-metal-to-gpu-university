#!/usr/bin/env bash
# F3-10 lab: milestone A1 (a UEFI application in C++ that prints the memory map, booted by OVMF
# in QEMU at two memory sizes) and milestone A5 (a legacy BIOS boot sector under SeaBIOS that
# prints the E820 memory map), plus the forensic evidence (a boot sector without its signature).
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
OVMF_CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS=/usr/share/OVMF/OVMF_VARS_4M.fd
EMU="hardware:  untested on hardware; run in QEMU 8.2.2 (q35) with the distribution's OVMF or SeaBIOS, not on a real PC"
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
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror"
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib"

# 1. build memmap.efi (PE32+ for UEFI) and show what kind of file it is
cmd="$UEFI_CXX -c memmap.cc -o memmap.o && $UEFI_LD /out:memmap.efi memmap.o && llvm-readobj --file-headers memmap.efi"
{
    $UEFI_CXX -c memmap.cc -o $B/memmap.o &&
    $UEFI_LD /out:$B/memmap.efi $B/memmap.o &&
    llvm-readobj --file-headers $B/memmap.efi | sed "s#$B/##" |
        grep -E 'File:|Format:|Arch:|Machine:|Magic:|Subsystem:|AddressOfEntryPoint:|ImageBase:'
} > build.out 2>&1; rc=$?
rec build "memmap.cc console.hpp efi.hpp" "$CLANGV" "$cmd" "$rc"
[ "$rc" = 0 ] || status=1

# 2. boot it twice: the ESP is a host folder that QEMU presents as a FAT disk; startup.nsh makes
#    the UEFI Shell run the app, print a line after it returns, and shut the machine down.
mkdir -p $B/esp
cp $B/memmap.efi $B/esp/memmap.efi
printf 'fs0:\r\nmemmap.efi\r\necho "A1: back in the UEFI Shell"\r\nreset -s\r\n' > $B/esp/startup.nsh
for mem in 256 512; do
    cp $OVMF_VARS $B/vars.fd
    qcmd="$QEMU -machine q35 -m ${mem}M -display none -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=OVMF_CODE_4M.fd -drive if=pflash,format=raw,file=OVMF_VARS_4M.fd(copy) -drive format=raw,file=fat:rw:esp"
    python3 qemu_run.py --timeout 60 -- $QEMU -machine q35 -m ${mem}M -display none -serial stdio -no-reboot -net none \
        -drive if=pflash,format=raw,readonly=on,file=$OVMF_CODE -drive if=pflash,format=raw,file=$B/vars.fd \
        -drive format=raw,file=fat:rw:$B/esp > memmap_${mem}.out 2>&1; rc=$?
    rec memmap_${mem} memmap.efi "$QEMUV; firmware: OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)" \
        "python3 qemu_run.py --timeout 60 -- $qcmd" "$rc (0 = the shell's reset -s powered the virtual machine off)" "$EMU"
    grep -q "A1: done" memmap_${mem}.out || status=1
done

# 3. acceptance check on the two runs
python3 check_a1.py 256 memmap_256.out 512 memmap_512.out > check_a1.out 2>&1; rc=$?
rec check_a1 check_a1.py "$(python3 --version)" "python3 check_a1.py 256 memmap_256.out 512 memmap_512.out" "$rc"
[ "$rc" = 0 ] || status=1

# 4. A5: the legacy BIOS boot sector under SeaBIOS (QEMU's default firmware for this machine)
NASMV="$(nasm -v)"
nasm -f bin bootsect.asm -o $B/disk.img > $B/nasm.txt 2>&1 || status=1
asm_size=$(stat -c %s $B/disk.img)
truncate -s 1M $B/disk.img   # SeaBIOS could not read a 1024-byte disk image; a 1 MiB image works
{
    echo "assembled size: $asm_size bytes (two sectors); disk image padded to $(stat -c %s $B/disk.img) bytes"
    echo "last two bytes of sector 0 (offsets 510-511):"
    od -A d -t x1 -j 510 -N 2 $B/disk.img
} > bootsect_image.out 2>&1; rc=$?
rec bootsect_image "bootsect.asm" "$NASMV" "nasm -f bin bootsect.asm -o disk.img; truncate -s 1M disk.img; od -A d -t x1 -j 510 -N 2 disk.img" "$rc"
qcmd="$QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive format=raw,file=disk.img -device isa-debug-exit,iobase=0xf4,iosize=0x04"
python3 qemu_run.py --timeout 20 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none \
    -drive format=raw,file=$B/disk.img -device isa-debug-exit,iobase=0xf4,iosize=0x04 > bootsect.out 2>&1; rc=$?
rec bootsect "bootsect.asm (disk.img)" "$NASMV; $QEMUV; firmware: SeaBIOS from the seabios package $(dpkg-query -W -f '${Version}' seabios)" \
    "python3 qemu_run.py --timeout 20 -- $qcmd" "$rc (33 = (0x10 << 1) | 1, written by stage 2 to the isa-debug-exit device)" "$EMU"
[ "$rc" = 33 ] || status=1

# 5. forensic evidence: the same image with the two signature bytes zeroed; SeaBIOS's own
#    debug messages (I/O port 0x402) are captured alongside the serial port
cp $B/disk.img $B/nosig.img
printf '\x00\x00' | dd of=$B/nosig.img bs=1 seek=510 conv=notrunc status=none
qcmd="$QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive format=raw,file=nosig.img -chardev file,id=dbg,path=seabios_nosig.txt -device isa-debugcon,iobase=0x402,chardev=dbg"
python3 qemu_run.py --timeout 8 -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none \
    -drive format=raw,file=$B/nosig.img -chardev file,id=dbg,path=$B/seabios_nosig.txt \
    -device isa-debugcon,iobase=0x402,chardev=dbg > forensic_serial.out 2>&1; rc=$?
rec forensic_serial "nosig.img (disk.img with bytes 510-511 set to 0)" "$QEMUV; SeaBIOS $(dpkg-query -W -f '${Version}' seabios)" \
    "python3 qemu_run.py --timeout 8 -- $qcmd" "$rc (124 = stopped by the 8 s time limit)" "$EMU"
tr -d '\r' < $B/seabios_nosig.txt > forensic_seabios.out
rec forensic_seabios "SeaBIOS debug port 0x402 of the same run" "$QEMUV; SeaBIOS $(dpkg-query -W -f '${Version}' seabios)" \
    "(file written by -chardev file,id=dbg,path=seabios_nosig.txt -device isa-debugcon,iobase=0x402,chardev=dbg)" "0" "$EMU"
# the healthy image's SeaBIOS log, for comparison
python3 qemu_run.py --timeout 20 -- $QEMU -machine q35 -m 256M -display none -serial null -no-reboot -net none \
    -drive format=raw,file=$B/disk.img -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -chardev file,id=dbg,path=$B/seabios_ok.txt -device isa-debugcon,iobase=0x402,chardev=dbg > /dev/null 2>&1
tr -d '\r' < $B/seabios_ok.txt > healthy_seabios.out
rec healthy_seabios "SeaBIOS debug port 0x402 while booting disk.img" "$QEMUV; SeaBIOS $(dpkg-query -W -f '${Version}' seabios)" \
    "$QEMU -machine q35 -m 256M -display none -serial null -no-reboot -net none -drive format=raw,file=disk.img -device isa-debug-exit,iobase=0xf4,iosize=0x04 -chardev file,id=dbg,path=seabios_ok.txt -device isa-debugcon,iobase=0x402,chardev=dbg" "0" "$EMU"
grep -q "Booting from Hard Disk" healthy_seabios.out || status=1

rm -rf "$B"
exit $status
