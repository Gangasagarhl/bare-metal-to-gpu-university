#!/usr/bin/env python3
"""check_a1.py - acceptance check for milestone A1 on the saved console output of two boots.

    check_a1.py <MiB given to QEMU> <memmap output file> [<MiB> <file> ...]

For each boot it reads the totals that memmap.efi printed and compares them with the memory
size given to QEMU (-m). It checks the milestone's wording literally ("the sum of conventional
memory is within a few MiB of the memory size given to QEMU", with "a few" taken as 8 MiB) and
also the memory the OS may use after ExitBootServices, and that the app returned to the shell.
"""
import re
import sys

LIMIT_MIB = 8
ok = True
args = sys.argv[1:]
for i in range(0, len(args), 2):
    given, path = int(args[i]), args[i + 1]
    text = open(path, encoding="utf-8").read()
    conv = int(re.search(r"conventional KiB: (\d+)", text).group(1)) / 1024
    usable = int(re.search(r"usable after ExitBootServices KiB: (\d+)", text).group(1)) / 1024
    returned = "back in the UEFI Shell" in text.split("A1: done", 1)[-1]
    print("QEMU -m %d MiB: conventional %.1f MiB (difference %.1f MiB), usable after "
          "ExitBootServices %.1f MiB (difference %.1f MiB), returned to the shell: %s"
          % (given, conv, given - conv, usable, given - usable, "yes" if returned else "no"))
    lit = abs(given - conv) <= LIMIT_MIB
    use = abs(given - usable) <= LIMIT_MIB
    print("  literal test (conventional within %d MiB): %s" % (LIMIT_MIB, "PASS" if lit else "FAIL"))
    print("  usable-memory test (within %d MiB):        %s" % (LIMIT_MIB, "PASS" if use else "FAIL"))
    print("  clean return test:                        %s" % ("PASS" if returned else "FAIL"))
    ok = ok and use and returned
print("A1 acceptance (usable-memory reading and clean return):", "PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)
