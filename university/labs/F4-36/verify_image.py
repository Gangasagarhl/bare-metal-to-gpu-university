#!/usr/bin/env python3
"""verify_image.py - F4-36: check, from the host, what the kernel's SD self-test wrote.

    python3 verify_image.py card.img FIRST COUNT

Recomputes the kernel's pseudo-random block numbers and data pattern (sdhci.cc, self_test and
pattern) and checks the image file: each written block must hold its pattern at the expected
place; block 0 must still carry the 0x55AA signature. If a block is missing, the checker also
looks at block number * 512, where a byte-addressed write to a block-addressed card would land.
"""
import struct
import sys

path, first, count = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])


def pattern(lba):
    return b"".join(struct.pack("<I", ((lba * 0x9E3779B1) ^ (i * 0x01010101)) & 0xFFFFFFFF) for i in range(128))


def block(f, lba):
    f.seek(lba * 512)
    return f.read(512)


seed, lbas = 12345, []
for _ in range(count):
    seed = (seed * 1103515245 + 12345) & 0xFFFFFFFF
    lbas.append(first + (seed >> 8) % (count * 4))
with open(path, "rb") as f:
    sig = block(f, 0)[510:512]
    good = sum(1 for lba in lbas if block(f, lba) == pattern(lba))
    elsewhere = sum(1 for lba in lbas if block(f, lba) != pattern(lba) and block(f, lba * 512) == pattern(lba))
print("block 0 signature: %s" % sig.hex())
print("blocks written by the self-test, found at the expected block: %d of %d" % (good, len(lbas)))
if good != len(lbas):
    print("missing blocks found instead at (block number x 512): %d of %d" % (elsewhere, len(lbas) - good))
    print("example: block %d expected at byte offset 0x%x; its data is at byte offset 0x%x"
          % (lbas[0], lbas[0] * 512, lbas[0] * 512 * 512))
sys.exit(0 if good == len(lbas) and sig == b"\x55\xaa" else 1)
