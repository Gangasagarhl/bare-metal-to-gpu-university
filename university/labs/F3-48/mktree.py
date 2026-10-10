#!/usr/bin/env python3
"""F3-48 mktree.py: the test tree for FS5 (deterministic contents, modes and names).

usage: mktree.py <dir>
"""
import os
import sys

root = sys.argv[1]
for d in ("docs", "boot/grub", "empty"):
    os.makedirs(os.path.join(root, d), exist_ok=True)
with open(os.path.join(root, "Read me first.txt"), "w") as f:
    f.write("Hello from the ISO\n")
with open(os.path.join(root, "docs/big.bin"), "wb") as f:
    f.write(bytes((i * 7) % 256 for i in range(70000)))
with open(os.path.join(root, "boot/grub/grub.cfg"), "w") as f:
    f.write("cfg\n")
with open(os.path.join(root, "docs/Übersicht – Ελληνικά.txt"), "w", encoding="utf-8") as f:
    f.write("Übung\n")
os.symlink("../Read me first.txt", os.path.join(root, "docs/link-to-readme"))
os.chmod(os.path.join(root, "docs/big.bin"), 0o600)
os.chmod(os.path.join(root, "boot/grub/grub.cfg"), 0o755)
for d in ("", "docs", "boot", "boot/grub", "empty"):
    os.chmod(os.path.join(root, d), 0o755)
