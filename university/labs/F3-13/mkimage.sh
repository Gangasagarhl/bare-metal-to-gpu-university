#!/usr/bin/env bash
# mkimage.sh - milestone A2: build a GPT disk image with an EFI System Partition, without root.
#
#   mkimage.sh <disk.img> <loader.efi> [<extra file for the ESP root> ...]
#
# Layout (128 MiB image, 512-byte sectors):
#   LBA 0            protective MBR            (written by sgdisk)
#   LBA 1            primary GPT header        (written by sgdisk)
#   LBA 2..33        primary partition entries (128 entries x 128 bytes)
#   partition 1      EFI System Partition, 64 MiB, FAT32, loader at \EFI\BOOT\BOOTX64.EFI
#   partition 2      "OS301 kernel data", the rest, reserved for the kernel's own file system
#   last 33 LBAs     backup partition entries and backup GPT header
# Fixed GUIDs and a fixed FAT volume ID make the image reproducible run after run.
set -euo pipefail
img=$1
loader=$2
shift 2
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

rm -f "$img"
truncate -s 128M "$img"
sgdisk --clear \
    --disk-guid=0a5301aa-0000-4000-8000-000000000001 \
    --new=1:2048:+64M --typecode=1:EF00 --change-name=1:"EFI system partition" \
    --partition-guid=1:0a5301aa-0000-4000-8000-0000000000e5 \
    --new=2:0:0 --typecode=2:8300 --change-name=2:"OS301 kernel data" \
    --partition-guid=2:0a5301aa-0000-4000-8000-0000000000d2 \
    "$img"

# The ESP is formatted as a separate file, filled with mtools, then copied into place.
esp="$work/esp.img"
truncate -s 64M "$esp"
mkfs.fat -F 32 -n OS301ESP -i 05301301 "$esp" > /dev/null
mmd -i "$esp" ::/EFI ::/EFI/BOOT
mcopy -i "$esp" "$loader" ::/EFI/BOOT/BOOTX64.EFI
for extra in "$@"; do
    mcopy -i "$esp" "$extra" ::/
done
dd if="$esp" of="$img" bs=512 seek=2048 conv=notrunc status=none
echo "mkimage: wrote $img"
