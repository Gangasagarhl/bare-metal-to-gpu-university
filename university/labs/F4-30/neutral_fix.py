#!/usr/bin/env python3
"""neutral_fix.py - F4-30: writes FIXED COPIES of two shared files into a scratch folder and
prints the unified diff. The originals are not edited: F3-18/kformat.h belongs to OS303 and
F4-23/neutral_tests.cc is the "before" state this chapter's forensic starts from. Whether the
owners adopt the fix is a decision recorded in DR402's NOTES.md.

The two bugs, both invisible on every 64-bit CPU (LP64: long and long long are 64 bits):
  1. F4-23's test passes 0xffffffff80000000ul with "%lx". On a 32-bit CPU (ILP32) that
     literal does not fit an unsigned long, so its type becomes unsigned long long: 8 bytes go
     into the variable-argument list, 4 come out, and every later argument is read from the
     wrong place (the next "%s" then reads half a number as a pointer).
  2. F3-18's kformat reads "%llx" exactly like "%lx" (any number of 'l' -> long), so the
     correct format would still be read wrongly on ILP32.
Usage: neutral_fix.py <labs dir> <out dir>
"""
import difflib
import os
import sys


def patch(text, old, new, what):
    if text.count(old) != 1:
        sys.exit("neutral_fix.py: cannot apply %s (pattern found %d times)" % (what, text.count(old)))
    return text.replace(old, new)


def main():
    labs, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    jobs = []

    src = open(os.path.join(labs, "F3-18", "kformat.h")).read()
    fixed = patch(src, "        bool is_long = false;\n", "        int longs = 0;                   // 0: int, 1: long, 2: long long\n",
                  "kformat: count the l's")
    fixed = patch(fixed, "        while (*p == 'l') {\n            is_long = true;\n",
                  "        while (*p == 'l') {\n            ++longs;\n", "kformat: count the l's (loop)")
    fixed = patch(fixed, "            int64_t v = is_long ? va_arg(ap, long) : va_arg(ap, int);\n",
                  "            int64_t v = longs >= 2 ? va_arg(ap, long long) : longs == 1 ? va_arg(ap, long) : va_arg(ap, int);\n",
                  "kformat: signed")
    fixed = patch(fixed, "            uint64_t v = is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);\n",
                  "            uint64_t v = longs >= 2   ? va_arg(ap, unsigned long long)\n"
                  "                         : longs == 1 ? va_arg(ap, unsigned long)\n"
                  "                                      : va_arg(ap, unsigned);\n",
                  "kformat: unsigned")
    jobs.append(("F3-18/kformat.h", src, fixed))

    src = open(os.path.join(labs, "F4-23", "neutral_tests.cc")).read()
    fixed = patch(src, '"[%5d] [%05u] [%x] [%lx] [%s] [%c] [%%]", -42, 99u, 0xbeefu,\n              0xffffffff80000000ul,',
                  '"[%5d] [%05u] [%x] [%llx] [%s] [%c] [%%]", -42, 99u, 0xbeefu,\n              0xffffffff80000000ull,',
                  "neutral_tests: 64-bit value, 64-bit format")
    jobs.append(("F4-23/neutral_tests.cc", src, fixed))

    for name, before, after in jobs:
        with open(os.path.join(out, os.path.basename(name)), "w") as f:
            f.write(after)
        sys.stdout.writelines(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                                   "labs/" + name, "fixed/" + os.path.basename(name), n=1))


if __name__ == "__main__":
    main()
