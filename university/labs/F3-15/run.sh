#!/usr/bin/env bash
# F3-15 lab: milestone A3. Our loader (loader.cc) loads our kernel stub (F3-14) from the ESP of
# our GPT image (F3-13), exits boot services and jumps. a3_test.py checks the acceptance tests
# against the running machine. Then: two damaged kernels, and the forensic evidence (a loader
# with a planted stale-map-key bug, two variants).
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
CLANGV="$(clang++ --version | head -n 1); $(lld-link --version | head -n 1)"
GXXV="$(g++ --version | head -n 1); $(ld.lld --version | head -n 1)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine (std VGA, AHCI disk) with the distribution's OVMF, not a real PC"
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
UEFI_CXX="clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10 -I ../F3-14"
UEFI_LD="lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib"
KFLAGS="-std=c++20 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-pic -fno-pie -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror"
QARGS="-machine q35 -m 256M -display none -no-reboot -net none"

# 1. build the loader (and its broken twin), the kernel, and the disk image
{
    $UEFI_CXX -c loader.cc -o $B/loader.o && $UEFI_LD /out:$B/loader.efi $B/loader.o &&
    $UEFI_CXX -DPLANT_STALE_KEY=1 -c loader.cc -o $B/stale1.o && $UEFI_LD /out:$B/stale1.efi $B/stale1.o &&
    $UEFI_CXX -DPLANT_STALE_KEY=2 -c loader.cc -o $B/stale2.o && $UEFI_LD /out:$B/stale2.efi $B/stale2.o &&
    g++ $KFLAGS -I ../F3-14 -c ../F3-14/kernel.cc -o $B/kernel.o &&
    ld.lld -T ../F3-14/kernel.ld -z max-page-size=4096 -nostdlib -static -o $B/kernel.elf $B/kernel.o &&
    printf 'console=serial boot-info-test\n' > $B/cmdline.txt &&
    bash ../F3-13/mkimage.sh $B/disk.img $B/loader.efi $B/kernel.elf $B/cmdline.txt > /dev/null 2>&1 &&
    ls -l $B/loader.efi $B/kernel.elf | awk '{print $NF ": " $5 " bytes"}' | sed "s#$B/##" &&
    echo "disk.img: GPT image from ../F3-13/mkimage.sh; ESP holds \\EFI\\BOOT\\BOOTX64.EFI (the loader), \\kernel.elf, \\cmdline.txt"
} > build.out 2>&1; rc=$?
rec build "loader.cc; ../F3-14/kernel.cc kernel.ld bootinfo.hpp elf64.hpp; ../F3-13/mkimage.sh" "$CLANGV; $GXXV" \
    "$UEFI_CXX -c loader.cc && $UEFI_LD /out:loader.efi loader.o; (same with -DPLANT_STALE_KEY=1 -> stale1.efi and -DPLANT_STALE_KEY=2 -> stale2.efi); g++ $KFLAGS -c kernel.cc && ld.lld -T kernel.ld ... -o kernel.elf; bash mkimage.sh disk.img loader.efi kernel.elf cmdline.txt" "$rc"
[ "$rc" = 0 ] || status=1

# 2. the acceptance test against the running machine
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 a3_test.py $B/kernel.elf $PWD/$B/mon.sock -- $QEMU $QARGS -serial stdio -monitor unix:$PWD/$B/mon.sock,server,nowait \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
    -drive format=raw,file=$B/disk.img > a3.out 2>&1; rc=$?
rec a3 "a3_test.py (machine: loader.efi + kernel.elf on disk.img)" "$(python3 --version); $QEMUV; $OVMFV" \
    "python3 a3_test.py kernel.elf <socket> -- $QEMU $QARGS -serial stdio -monitor unix:<socket>,server,nowait -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file=disk.img" \
    "$rc (0 = every acceptance check passed)" "$EMU"
[ "$rc" = 0 ] || status=1

# 3. damaged kernels: bad magic, and two segments at the same address
python3 -c "import sys; d=bytearray(open(sys.argv[1],'rb').read()); d[0]=0x58; open(sys.argv[2],'wb').write(d)" \
    $B/kernel.elf $B/badmagic.elf
python3 -c "import sys,struct; d=bytearray(open(sys.argv[1],'rb').read()); v=struct.unpack_from('<Q',d,64+16)[0]; struct.pack_into('<Q',d,64+56+16,v); open(sys.argv[2],'wb').write(d)" \
    $B/kernel.elf $B/overlap.elf
for bad in badmagic overlap; do
    cp $B/$bad.elf $B/kernel_bad.elf
    mkdir -p $B/$bad && cp $B/$bad.elf $B/$bad/kernel.elf
    bash ../F3-13/mkimage.sh $B/$bad.img $B/loader.efi $B/$bad/kernel.elf $B/cmdline.txt > /dev/null 2>&1
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 60 --stop-after "returning to the firmware" -- $QEMU $QARGS -serial stdio \
        -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
        -drive format=raw,file=$B/$bad.img > bad_$bad.out 2>&1; rc=$?
    rec bad_$bad "loader.efi with a damaged kernel.elf ($bad)" "$QEMUV; $OVMFV" \
        "python3 ../F3-10/qemu_run.py --timeout 60 --stop-after 'returning to the firmware' -- $QEMU $QARGS -serial stdio -drive if=pflash,... -drive format=raw,file=$bad.img" \
        "$rc (124 = the harness stopped QEMU 0.5 s after the loader gave control back)" "$EMU"
    grep -q "rejected" bad_$bad.out || status=1
done

# 4. forensic evidence: the loader with a planted bug ("stuck after the logo"), two variants:
#    1 = a 64-byte AllocatePool between GetMemoryMap and ExitBootServices
#    2 = a one-page AllocatePages between GetMemoryMap and ExitBootServices
mkdir -p $B/st && cp $B/kernel.elf $B/st/kernel.elf
for v in 1 2; do
    cp $B/stale$v.efi $B/stale_loader.efi
    bash ../F3-13/mkimage.sh $B/stale$v.img $B/stale_loader.efi $B/st/kernel.elf $B/cmdline.txt > /dev/null 2>&1
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 25 --stamp --stop-after "kernel: halting" -- $QEMU $QARGS -serial stdio \
        -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
        -drive format=raw,file=$B/stale$v.img > forensic_stale$v.out 2>&1; rc=$?
    rec forensic_stale$v "loader.cc built with -DPLANT_STALE_KEY=$v" "$CLANGV; $QEMUV; $OVMFV" \
        "python3 ../F3-10/qemu_run.py --timeout 25 --stamp --stop-after 'kernel: halting' -- $QEMU $QARGS -serial stdio -drive if=pflash,... -drive format=raw,file=stale$v.img" \
        "$rc (124 = stopped by the harness: after 'kernel: halting', or by the 25 s time limit when the machine never got further)" "$EMU; first column: host seconds since QEMU started"
done
grep -q "kernel: halting" forensic_stale1.out || status=1      # observed: variant 1 does not trip on this firmware
grep -q "ExitBootServices failed" forensic_stale2.out || status=1
cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
python3 ../F3-10/qemu_run.py --timeout 25 --stamp --stop-after "kernel: halting" -- $QEMU $QARGS -serial stdio \
    -drive if=pflash,format=raw,readonly=on,file=$CODE -drive if=pflash,format=raw,file=$B/vars.fd \
    -drive format=raw,file=$B/disk.img > forensic_healthy.out 2>&1; rc=$?
rec forensic_healthy "loader.cc (normal build)" "$CLANGV; $QEMUV; $OVMFV" \
    "python3 ../F3-10/qemu_run.py --timeout 25 --stamp --stop-after 'kernel: halting' -- $QEMU $QARGS -serial stdio -drive if=pflash,... -drive format=raw,file=disk.img" \
    "$rc (124 = the harness stopped QEMU 0.5 s after the kernel halted)" "$EMU; first column: host seconds since QEMU started"

rm -rf "$B"
exit $status
