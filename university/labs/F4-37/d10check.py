#!/usr/bin/env python3
"""d10check.py - F4-37: milestone D10's first acceptance test, in the form this build can run.

    python3 d10check.py SHA_VIRT SHA_RPI virt.out EXPECT_MIB EXPECT_CPUS rpi.out EXPECT_MIB EXPECT_CPUS

One kernel file, byte-identical (same SHA-256), boots on both machines; on both, the serial log
shows memory size, CPU count and timer frequency matching the platform. "Matching the platform"
is checked against what the QEMU command line asked for (virt) and against the devicetree the
loader handed over (raspi3b: QEMU rewrites the memory node; see rpi_dtb.out). The timer
frequency is checked for consistency: the value the kernel printed must equal the value its
timer driver measured against."""
import re
import sys

sha_a, sha_b = sys.argv[1], sys.argv[2]
ok = sha_a == sha_b
print("image SHA-256 on virt:    %s" % sha_a)
print("image SHA-256 on raspi3b: %s  -> %s" % (sha_b, "identical" if ok else "DIFFERENT"))
for log, mib, cpus in ((sys.argv[3], sys.argv[4], sys.argv[5]), (sys.argv[6], sys.argv[7], sys.argv[8])):
    text = open(log).read()
    m = re.search(r"memory (\d+) MiB, CPUs (\d+), timer (\d+) Hz", text)
    t = re.search(r"CNTFRQ_EL0 (\d+) Hz", text)
    good = bool(m and t and m.group(1) == mib and m.group(2) == cpus and m.group(3) == t.group(1)
                and "DR403 kernel: done" in text)
    print("%-14s memory %s MiB (expected %s), CPUs %s (expected %s), timer %s Hz -> %s"
          % (log, m.group(1) if m else "?", mib, m.group(2) if m else "?", cpus, m.group(3) if m else "?",
             "PASS" if good else "FAIL"))
    ok = ok and good
print("D10 acceptance test 1 (this build's form): %s" % ("PASS" if ok else "FAIL"))
sys.exit(0 if ok else 1)
