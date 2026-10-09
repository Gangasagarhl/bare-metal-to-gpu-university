#!/usr/bin/env python3
"""F3-48 hostlist.py: list a host tree in the formats of "isoread ls".

usage: hostlist.py <dir> rr|joliet
  rr:     d <path> <mode>; f <path> <mode> <size> <crc32>; l <path> -> <target>
  joliet: d <path>;        f <path> <size> <crc32>        (Joliet has no modes, no links)
"""
import os
import stat
import sys
import zlib

root, kind = sys.argv[1], sys.argv[2]
out = []
for dp, dns, fns in os.walk(root):
    rel = dp[len(root):]
    for n in dns + fns:
        p = os.path.join(dp, n)
        st = os.lstat(p)
        path = rel + "/" + n
        if stat.S_ISLNK(st.st_mode):
            if kind == "rr":
                out.append((path, "l %s -> %s" % (path, os.readlink(p))))
        elif stat.S_ISDIR(st.st_mode):
            out.append((path, "d %s %o" % (path, st.st_mode) if kind == "rr" else "d %s" % path))
        else:
            data = open(p, "rb").read()
            mode = " %o" % st.st_mode if kind == "rr" else ""
            out.append((path, "f %s%s %d %08x" % (path, mode, len(data), zlib.crc32(data))))
for _, line in sorted(out):
    print(line)
