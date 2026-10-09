#!/usr/bin/env bash
# F3-13 lab: milestone A2. Build a GPT disk image with an ESP (mkimage.sh, no root needed),
# verify it with our own C++ tool (gptcheck.cc) and with sgdisk, boot it with OVMF, corrupt one
# byte of the primary header and observe both the verifier and the firmware, and produce the
# forensic evidence (the same image copied onto a larger disk).
# Every step writes <name>.out (real output) and <name>.log (run record).
set -u -o pipefail
cd "$(dirname "$0")"
status=0
B=.build
rm -rf "$B"; mkdir -p "$B"
QEMU=qemu-system-x86_64
QEMUV="$($QEMU --version | head -n 1)"
OVMFV="OVMF from the ovmf package $(dpkg-query -W -f '${Version}' ovmf)"
GXX="g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined"
GXXV="$(g++ --version | head -n 1)"
TOOLV="$(sgdisk --version | head -n 1 | tr -d '\r'); mkfs.fat $(dpkg-query -W -f '${Version}' dosfstools); mtools $(dpkg-query -W -f '${Version}' mtools)"
CODE=/usr/share/OVMF/OVMF_CODE_4M.fd
EMU="hardware:  untested on hardware; QEMU 8.2.2 q35 machine (AHCI disk) with the distribution's OVMF, not a real PC or USB stick"
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
boot() {  # boot <image> -> console text on stdout; stops 0.5 s after A1 finishes
    cp /usr/share/OVMF/OVMF_VARS_4M.fd $B/vars.fd
    python3 ../F3-10/qemu_run.py --timeout 60 --stop-after "A1: done" -- $QEMU -machine q35 -m 256M -display none \
        -serial stdio -no-reboot -net none -drive if=pflash,format=raw,readonly=on,file=$CODE \
        -drive if=pflash,format=raw,file=$B/vars.fd -drive format=raw,file="$1"
}
BOOTCMD="python3 ../F3-10/qemu_run.py --timeout 60 --stop-after 'A1: done' -- $QEMU -machine q35 -m 256M -display none -serial stdio -no-reboot -net none -drive if=pflash,...OVMF_CODE_4M.fd -drive if=pflash,...OVMF_VARS_4M.fd(copy) -drive format=raw,file="

# 1. the loader we put on the ESP is A1's memmap.efi (F3-10), and the verifier is built for the host
clang++ --target=x86_64-unknown-windows -std=c++20 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone \
    -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10 -c ../F3-10/memmap.cc -o $B/memmap.o &&
    lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /out:$B/memmap.efi $B/memmap.o > /dev/null || status=1
$GXX gptcheck.cc -o $B/gptcheck || status=1

# 2. build the image
bash mkimage.sh $B/disk.img $B/memmap.efi 2>&1 | sed "s#$B/##" > mkimage.out; rc=${PIPESTATUS[0]}
rec mkimage mkimage.sh "$TOOLV" "bash mkimage.sh disk.img memmap.efi" "$rc"
[ "$rc" = 0 ] || status=1

# 3. verify it: our tool, then sgdisk's own check and table
(cd $B && ./gptcheck disk.img) > gptcheck.out 2>&1; rc=$?
rec gptcheck gptcheck.cc "$GXXV" "$GXX gptcheck.cc -o gptcheck; ./gptcheck disk.img" "$rc"
[ "$rc" = 0 ] || status=1
(cd $B && sgdisk -v disk.img && sgdisk -p disk.img) > sgdisk.out 2>&1; rc=$?
rec sgdisk "(no listing: sgdisk)" "$TOOLV" "sgdisk -v disk.img && sgdisk -p disk.img" "$rc"

# 4. boot it: OVMF finds \EFI\BOOT\BOOTX64.EFI on its own (no boot menu interaction)
cp $B/disk.img $B/boot.img
boot $B/boot.img > boot_gpt.out 2>&1; rc=$?
rec boot_gpt "disk.img (memmap.efi as BOOTX64.EFI)" "$QEMUV; $OVMFV" "${BOOTCMD}disk.img" \
    "$rc (124 = the harness stopped QEMU 0.5 s after the line 'A1: done')" "$EMU"
grep -q "A1: done" boot_gpt.out || status=1

# 5. corrupt one byte of the primary GPT header (offset 512 + 56: first byte of the disk GUID)
cp $B/disk.img $B/corrupt.img
printf '\x00' | dd of=$B/corrupt.img bs=1 seek=$((512 + 56)) conv=notrunc status=none
(cd $B && ./gptcheck corrupt.img) > corrupt_check.out 2>&1; rc=$?
rec corrupt_check gptcheck.cc "$GXXV" "./gptcheck corrupt.img   (disk.img with byte 568 set to 0x00)" "$rc (1 = verifier failed, as intended)"
[ "$rc" = 1 ] || status=1
boot $B/corrupt.img > corrupt_boot.out 2>&1; rc=$?
rec corrupt_boot "corrupt.img" "$QEMUV; $OVMFV" "${BOOTCMD}corrupt.img" \
    "$rc (124 = the harness stopped QEMU 0.5 s after the line 'A1: done')" "$EMU"
grep -q "A1: done" corrupt_boot.out || status=1
(cd $B && ./gptcheck corrupt.img) > corrupt_after_boot.out 2>&1; rc=$?
rec corrupt_after_boot gptcheck.cc "$GXXV" "./gptcheck corrupt.img   (the same file, after OVMF booted from it)" "$rc"

# 6. forensic evidence: the image written to a larger disk (here: the file grown to 192 MiB)
cp $B/disk.img $B/grown.img
truncate -s 192M $B/grown.img
(cd $B && ./gptcheck grown.img) > forensic_check.out 2>&1; rc=$?
rec forensic_check gptcheck.cc "$GXXV" "./gptcheck grown.img   (disk.img grown with truncate -s 192M)" "$rc"
(cd $B && sgdisk -v grown.img) > forensic_sgdisk.out 2>&1; rc=$?
rec forensic_sgdisk "(no listing: sgdisk)" "$TOOLV" "sgdisk -v grown.img" "$rc"
cp $B/grown.img $B/grown_before.img
boot $B/grown.img > forensic_boot.out 2>&1; rc=$?
cmp -s $B/grown.img $B/grown_before.img && echo "after the boot: grown.img is byte-for-byte unchanged (cmp)" >> forensic_boot.out \
    || echo "after the boot: grown.img was changed by the firmware (cmp)" >> forensic_boot.out
rec forensic_boot "grown.img" "$QEMUV; $OVMFV" "${BOOTCMD}grown.img; cmp grown.img grown_before.img" \
    "$rc (124 = the harness stopped QEMU 0.5 s after the line 'A1: done')" "$EMU"
(cd $B && sgdisk -e grown.img > /dev/null && ./gptcheck grown.img) > forensic_fixed.out 2>&1; rc=$?
rec forensic_fixed "gptcheck.cc" "$TOOLV; $GXXV" "sgdisk -e grown.img && ./gptcheck grown.img" "$rc"
[ "$rc" = 0 ] || status=1

rm -rf "$B"
exit $status
