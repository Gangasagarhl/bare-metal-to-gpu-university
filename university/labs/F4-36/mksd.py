#!/usr/bin/env python3
"""mksd.py - F4-36: make an SD card image for QEMU.

    python3 mksd.py card.img SIZE_MIB

The image is sparse (only written blocks use disk space). Block 0 gets a partition table in the
classic MBR layout - one entry at byte 446 (status, CHS fields left 0, type 0x0c, first LBA,
number of sectors) and the signature 0x55 0xAA at bytes 510-511 - with partition 1 starting at
block 8192. Blocks 1-8191 stay unused: the kernel's write test uses blocks 2048-2175 there, so it
never touches a partition. (MBR layout written from memory of the format; QEMU does not read it,
only our kernel and verify_image.py do.)
"""
import struct
import sys

path, mib = sys.argv[1], int(sys.argv[2])
blocks = mib * 2048
with open(path, "wb") as f:
    f.truncate(blocks * 512)
    entry = struct.pack("<B3sB3sII", 0x00, b"\0\0\0", 0x0C, b"\0\0\0", 8192, blocks - 8192)
    f.seek(446)
    f.write(entry)
    f.seek(510)
    f.write(b"\x55\xaa")
print("%s: %d MiB, %d blocks; partition 1 (type 0x0c) from block 8192; scratch area 2048-2175 unused"
      % (path, mib, blocks))
