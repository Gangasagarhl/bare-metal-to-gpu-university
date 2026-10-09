#!/usr/bin/env python3
"""F3-46 hostlist.py: list a host directory tree the way "ext4read tree" does (type, path,
size, zlib CRC-32), so the two listings can be compared with diff."""
import os, sys, zlib
root = sys.argv[1]
out = []
for dp, dns, fns in os.walk(root):
    rel = dp[len(root):]
    for d in dns:
        p = os.path.join(dp, d)
        if os.path.islink(p):
            t = os.readlink(p).encode()
            out.append("l %s/%s %d %08x" % (rel, d, len(t), zlib.crc32(t)))
        else:
            out.append("d %s/%s" % (rel, d))
    for f in fns:
        p = os.path.join(dp, f)
        if os.path.islink(p):
            t = os.readlink(p).encode(); kind = "l"
        else:
            t = open(p, "rb").read(); kind = "f"
        out.append("%s %s/%s %d %08x" % (kind, rel, f, len(t), zlib.crc32(t)))
for line in sorted(out, key=lambda l: l.split(" ")[1]):
    print(line)
