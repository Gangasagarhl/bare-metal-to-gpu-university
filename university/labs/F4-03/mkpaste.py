#!/usr/bin/env python3
"""Write the 64 KiB paste test file (every byte value, pseudo-random order, fixed seed)
and print its FNV-1a hash in the same format as the kernel."""
import random
import sys

data = bytes(random.Random(301).randrange(256) for _ in range(65536))
open(sys.argv[1], "wb").write(data)
h = 2166136261
for b in data:
    h = ((h ^ b) * 16777619) & 0xFFFFFFFF
print("host:  %d bytes, fnv1a %08x" % (len(data), h))
