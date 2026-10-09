#!/usr/bin/env python3
"""mkflash.py - F4-32: build the flash image the "boot ROM" reads.

    python3 mkflash.py out.bin rom.bin bl2.bin:0x0e000000 bl31.bin:0x0e100000 bl33.bin:0x40080000 \
                       [--corrupt NAME]

Layout (this course's own, fixed in rom.cc and bl2.cc): ROM code at offset 0, the BL2 image at
0x10000, BL31 at 0x20000, BL33 at 0x40000; each image is a 32-byte header (see bootimg.h) plus
its payload. The file is padded to 64 MiB, the size of QEMU's first flash device. --corrupt NAME
flips one bit in the middle of that image's payload after its CRC-32 was computed.
"""
import struct
import sys
import zlib

OFFSETS = {"BL2": 0x10000, "BL31": 0x20000, "BL33": 0x40000}
FLASH_SIZE = 64 * 1024 * 1024


def main():
    args = sys.argv[1:]
    corrupt = None
    if "--corrupt" in args:
        i = args.index("--corrupt")
        corrupt = args[i + 1]
        del args[i:i + 2]
    out, rom, images = args[0], args[1], args[2:]
    flash = bytearray(FLASH_SIZE)
    code = open(rom, "rb").read()
    assert len(code) < OFFSETS["BL2"], "ROM too large"
    flash[0:len(code)] = code
    for spec, name in zip(images, ("BL2", "BL31", "BL33")):
        path, load = spec.split(":")
        payload = bytearray(open(path, "rb").read())
        header = struct.pack("<4sIII16s", b"DR4I", int(load, 0), len(payload),
                             zlib.crc32(payload) & 0xFFFFFFFF, name.encode())
        if name == corrupt:
            payload[len(payload) // 2] ^= 0x01
        off = OFFSETS[name]
        assert off + 32 + len(payload) <= FLASH_SIZE
        flash[off:off + 32] = header
        flash[off + 32:off + 32 + len(payload)] = payload
        crc = zlib.crc32(bytes(payload)) & 0xFFFFFFFF
        print("%-4s %6d bytes, load 0x%08x, header CRC-32 0x%08x, at flash offset 0x%05x%s"
              % (name, len(payload), int(load, 0), struct.unpack("<I", header[12:16])[0], off,
                 "  (one bit flipped: payload CRC-32 is now 0x%08x)" % crc if name == corrupt else ""))
    open(out, "wb").write(flash)


if __name__ == "__main__":
    main()
