#!/usr/bin/env python3
"""hexdump.py - F10-32: print the first N bytes of a file as hex and ASCII, 16 per line.
usage: hexdump.py FILE [N]"""
import sys

data = open(sys.argv[1], "rb").read()[:int(sys.argv[2]) if len(sys.argv) > 2 else 96]
for i in range(0, len(data), 16):
    row = data[i:i + 16]
    print(f"{i:04x}  " + " ".join(f"{b:02x}" for b in row) + "  " +
          "".join(chr(b) if 32 <= b < 127 else "." for b in row))
