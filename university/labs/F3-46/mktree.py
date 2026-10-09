#!/usr/bin/env python3
"""F3-46 mktree.py: the test tree for FS3 (deterministic contents).

usage: mktree.py <dir>
"""
import os
import sys

root = sys.argv[1]
for d in ("docs", "big", "many", "docs/deeper"):
    os.makedirs(os.path.join(root, d), exist_ok=True)
open(os.path.join(root, "docs/a.txt"), "w").write("hi\n")
open(os.path.join(root, "docs/deeper/empty.txt"), "w").close()
open(os.path.join(root, "docs/notes.txt"), "w").write("".join("line %d\n" % i for i in range(2000)))
# 3 MiB + 123 bytes: several extents' worth of contiguous blocks
open(os.path.join(root, "big/data.bin"), "wb").write(bytes((i * 31 + 7) % 256 for i in range(3 * 1024 * 1024 + 123)))
# twelve 5000-byte islands, 300000 bytes apart: a sparse file with more than four extents
with open(os.path.join(root, "big/sparse.bin"), "wb") as f:
    for k in range(12):
        f.seek(k * 300000)
        f.write(bytes([k + 1]) * 5000)
# 2000 small files in one directory: big enough to be indexed (a hashed tree)
for i in range(2000):
    open(os.path.join(root, "many/file%04d.txt" % i), "w").write("n%d\n" % i)
os.symlink("docs/a.txt", os.path.join(root, "short"))          # fast symlink (< 60 bytes)
os.symlink("x" * 100, os.path.join(root, "long"))              # slow symlink (in a block)
os.link(os.path.join(root, "docs/a.txt"), os.path.join(root, "docs/hardlink.txt"))
