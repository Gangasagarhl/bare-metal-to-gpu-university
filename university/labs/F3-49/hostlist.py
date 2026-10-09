#!/usr/bin/env python3
"""F3-49 hostlist.py: list a host tree the way "ntfsread ls" does (d <path>; f <path> <size> <crc32>)."""
import os
import sys
import zlib

root = sys.argv[1]
out = []
for dp, dns, fns in os.walk(root):
    for n in dns + fns:
        p = os.path.join(dp, n)
        path = p[len(root):]
        if os.path.isdir(p):
            out.append("d %s" % path)
        else:
            data = open(p, "rb").read()
            out.append("f %s %d %08x" % (path, len(data), zlib.crc32(data)))
for line in sorted(out):
    print(line)
