#!/usr/bin/env bash
# mkimage.sh - F9-67: pack the robot image into one disk image file.
#   mkimage.sh OUT.img Image robot.dtb .config
# Layout (GPT): partition 1 "boot" (FAT) holds the kernel, the devicetree, the resolved
# configuration and a manifest of SHA-256 checksums; partition 2 "rootfs-b" is the empty slot a
# later update would fill (A/B updates, F9-67 Layer 3); partition 3 "logs" keeps logs across
# updates (F9-70). Tools: sgdisk, mkfs.vfat and mtools (no root rights and no loop devices needed).
set -eu -o pipefail
out="$1"; kernel="$2"; dtb="$3"; config="$4"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$kernel" "$work/Image"; cp "$dtb" "$work/robot.dtb"; cp "$config" "$work/robot.cfg"
(cd "$work" && sha256sum Image robot.dtb robot.cfg > manifest.txt)
# 1. the boot file system, built as a separate 8 MiB file
truncate -s 8M "$work/boot.fat"
mkfs.vfat -n ROBOTBOOT "$work/boot.fat" > /dev/null
for f in Image robot.dtb robot.cfg manifest.txt; do
    mcopy -i "$work/boot.fat" "$work/$f" "::$f"
done
# 2. the disk: 32 MiB with a GPT; sizes in 512-byte sectors (sgdisk 1.0.10 wrote nothing when
#    given -q in this build, so its messages are discarded instead)
rm -f "$out"
truncate -s 32M "$out"
sgdisk -n 1:2048:+8M -t 1:0700 -c 1:boot \
          -n 2:0:+12M  -t 2:8300 -c 2:rootfs-b \
          -n 3:0:0     -t 3:8300 -c 3:logs "$out" > /dev/null 2>&1
# 3. copy the file system into partition 1 (it starts at sector 2048 = 1 MiB)
dd if="$work/boot.fat" of="$out" bs=1M seek=1 conv=notrunc status=none
echo "mkimage: wrote $out"
