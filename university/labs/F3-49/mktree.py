#!/usr/bin/env python3
"""F3-49 mktree.py: the host tree that ntfsgen turns into the FS6 test image.

usage: mktree.py <dir>
Small files stay resident in their MFT record; "notes.txt" is resident but longer than the
first 512-byte stride of its record; "fragmented.bin" and "sparse.vhd" trigger ntfsgen's
three-run and sparse layouts; "Photos" holds enough names to need INDX blocks.
"""
import os
import sys

root = sys.argv[1]
for d in ("Documents", "Photos", "Empty folder"):
    os.makedirs(os.path.join(root, d), exist_ok=True)


def put(path, data):
    with open(os.path.join(root, path), "wb") as f:
        f.write(data)


put("Read me.txt", b"This volume was written by ntfsgen for OS401.\n")
put("notes.txt", b"".join(b"line %02d: the quick brown fox jumps over the lazy dog\n" % i for i in range(12)))
put("Documents/report.pdf", bytes((i * 31 + 7) % 251 for i in range(50000)))
put("Documents/fragmented.bin", bytes((i * 7 + i // 4096) % 256 for i in range(40000)))
put("Documents/sparse.vhd", b"HEAD" * 1024 + bytes(512 * 1024 - 4096) + b"MID!" * 1024 + bytes(512 * 1024 - 4096))
put("Documents/Übersicht – Ελληνικά.txt", "Grüße, καλημέρα\n".encode())
for i in range(1, 151):
    put("Photos/IMG_%04d.JPG" % i, b"\xff\xd8\xff\xe0 photo %d \xff\xd9" % i)
