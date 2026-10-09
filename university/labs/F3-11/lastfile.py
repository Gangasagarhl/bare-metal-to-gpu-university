#!/usr/bin/env python3
"""lastfile.py - which firmware file holds the reset vector?

    lastfile.py <firmware image> <volume offset>

Walks the firmware files of the volume at <volume offset> (same header rules as fvscan.cc:
24-byte file headers, 8-byte alignment, type at byte 18, 24-bit size at byte 20; unverified, see
F3-11) and prints the last one, with the file offsets it covers, and whether the image's last
16 bytes (the bytes the CPU fetches first, F3-09) lie inside it.
"""
import sys

data = open(sys.argv[1], "rb").read()
fv = int(sys.argv[2], 0)
length = int.from_bytes(data[fv + 32:fv + 40], "little")
off = fv + int.from_bytes(data[fv + 48:fv + 50], "little")
ext = int.from_bytes(data[fv + 52:fv + 54], "little")
if ext:
    off = fv + ext + int.from_bytes(data[fv + ext + 16:fv + ext + 20], "little")
last = None
while True:
    off = fv + ((off - fv + 7) & ~7)
    if off + 24 > fv + length or data[off:off + 24] == b"\xff" * 24:
        break
    size = int.from_bytes(data[off + 20:off + 23], "little")
    last = (off, size, data[off + 18], data[off:off + 4][::-1].hex())
    off += size
start, size, ftype, guid = last
print("volume at 0x%x, length 0x%x (ends at 0x%x); image size 0x%x" % (fv, length, fv + length, len(data)))
print("last file: GUID %s..., type 0x%02x, %d bytes, file offsets 0x%x-0x%x"
      % (guid, ftype, size, start, start + size - 1))
inside = start <= len(data) - 16 and start + size == len(data)
print("the image's last 16 bytes (offsets 0x%x-0x%x) are the last 16 bytes of this file: %s"
      % (len(data) - 16, len(data) - 1, "yes" if inside else "no"))
print("those bytes: %s" % data[-16:].hex(" "))
