#!/usr/bin/env python3
"""Build the project's root file system image: a USTAR archive, padded to a whole number
of 4 KiB blocks, and print the FNV-1a of every file (the kernel must print the same).

usage: mkroot.py OUT_IMAGE > root.expect"""
import io
import random
import sys
import tarfile


def fnv1a(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


rng = random.Random(301)
files = {
    "etc/hostname": b"dr301-nvme\n",
    "etc/motd": b"Root file system read from NVMe namespace 2 by the DR301 lab kernel.\n",
    "bin/init": b"#!/bin/sh\n# placeholder: the init program arrives with Track U\necho hello from init\n",
    "data/big.bin": bytes(rng.getrandbits(8) for _ in range(1024 * 1024 + 333)),
}
buf = io.BytesIO()
with tarfile.open(fileobj=buf, mode="w", format=tarfile.USTAR_FORMAT) as tar:
    for d in ("bin", "data", "etc"):
        info = tarfile.TarInfo(d)
        info.type = tarfile.DIRTYPE
        info.mode = 0o755
        info.mtime = 0
        tar.addfile(info)
    for name in sorted(files):
        info = tarfile.TarInfo(name)
        info.size = len(files[name])
        info.mode = 0o755 if name.startswith("bin/") else 0o644
        info.mtime = 0
        tar.addfile(info, io.BytesIO(files[name]))
data = buf.getvalue()
data += b"\0" * (-len(data) % (1024 * 1024))          # whole MiB: whole 4 KiB blocks
open(sys.argv[1], "wb").write(data)
for name in sorted(files):
    print("root: %s %d bytes fnv1a %08x" % (name, len(files[name]), fnv1a(files[name])))
