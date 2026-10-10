#!/usr/bin/env python3
"""uefi_clean.py - F4-33: turn QEMU's raw serial output from the UEFI firmware into plain text.
Cursor-positioning escape sequences become line breaks, other escape sequences and carriage
returns are removed, blank lines dropped, and the UEFI
Shell's countdown before startup.nsh is replaced by one line marked [...] (trimming, AH-25).
Reads standard input, writes standard output."""
import re
import sys

data = sys.stdin.buffer.read().decode("utf-8", "replace")
data = re.sub(r"\x1b\[[0-9;?=]*([A-Za-z])", lambda m: "\n" if m.group(1) in "HJf" else "", data)
data = data.replace("\r", "")
out = []
for line in data.split("\n"):
    if not line.strip():
        continue
    if "to skip startup.nsh" in line:
        line = "[...] (UEFI Shell countdown before startup.nsh, trimmed)"
    if out and out[-1] == line:
        continue
    out.append(line.rstrip())
print("\n".join(out))
