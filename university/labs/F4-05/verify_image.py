#!/usr/bin/env python3
"""Check a disk image after the random-I/O test: replay the kernel's generator (the same
xorshift32 and seeds), find the last generation written to each block, and compare every
written block with the expected pattern; unwritten blocks must still be zero.

usage: verify_image.py IMAGE SEED[,SEED...] OPS"""
import struct
import sys

img, seeds, ops = sys.argv[1], [int(s, 0) for s in sys.argv[2].split(",")], int(sys.argv[3])
BLOCKS = 262144
M = 0xFFFFFFFF


def xorshift(x):
    x ^= (x << 13) & M
    x ^= x >> 17
    x ^= (x << 5) & M
    return x


gen = {}
for seed in seeds:
    x = seed
    for _ in range(ops):
        x = xorshift(x)
        block = x % BLOCKS
        x = xorshift(x)
        if x & 1:
            gen[block] = gen.get(block, 0) % 255 + 1

bad = 0
with open(img, "rb") as f:
    for block, g in sorted(gen.items()):
        f.seek(block * 4096)
        data = f.read(4096)
        want = struct.pack("<1024I", *(((block * 0x9E3779B1) + g * 0x85EBCA77 + i * 0xC2B2AE3D) & M
                                       for i in range(1024)))
        if data != want:
            bad += 1
    import os
    size = os.path.getsize(img)
    # sample of unwritten blocks: every 997th block that the test never wrote
    zero_bad = 0
    sampled = 0
    for block in range(0, BLOCKS, 997):
        if block in gen:
            continue
        f.seek(block * 4096)
        sampled += 1
        if any(f.read(4096)):
            zero_bad += 1
print("image %d bytes; %d distinct blocks written by the test; %d differ from the expected pattern"
      % (size, len(gen), bad))
print("%d sampled never-written blocks; %d are not zero" % (sampled, zero_bad))
print("image check: %s" % ("PASS" if bad == 0 and zero_bad == 0 else "FAIL"))
sys.exit(0 if bad == 0 and zero_bad == 0 else 1)
