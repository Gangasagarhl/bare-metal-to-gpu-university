#!/usr/bin/env python3
"""fvdiff.py - compare the module lists of two firmware builds, as printed by fvscan (F3-11).

    fvdiff.py <fvscan output A> <fvscan output B>

A module is identified by its PI file type and its name (the user-interface section); files
without a name are counted but not compared. Prints the modules found only in A, only in B,
and a count per type for both.
"""
import collections
import re
import sys


def modules(path):
    found = collections.Counter()
    for line in open(path, encoding="utf-8"):
        m = re.match(r"\s+([A-Z_]+)\s+\d+ bytes\s+[0-9a-f]{8}\s+(\S+)\s*$", line)
        if m:
            found[(m.group(1), m.group(2))] += 1
    return found


def main():
    a_path, b_path = sys.argv[1], sys.argv[2]
    a, b = modules(a_path), modules(b_path)
    print("named modules: %d in %s, %d in %s" % (sum(a.values()), a_path, sum(b.values()), b_path))
    for title, mine, other in (("only in " + a_path, a, b), ("only in " + b_path, b, a)):
        extra = sorted(k for k in mine if k not in other)
        print("%s (%d):" % (title, len(extra)))
        for ftype, name in extra:
            print("  %-22s %s" % (ftype, name))
    types = sorted({t for t, _ in a} | {t for t, _ in b})
    print("per type:  %-22s %6s %6s" % ("", "A", "B"))
    for t in types:
        print("           %-22s %6d %6d" % (t, sum(v for (k, _), v in a.items() if k == t),
                                            sum(v for (k, _), v in b.items() if k == t)))


if __name__ == "__main__":
    main()
