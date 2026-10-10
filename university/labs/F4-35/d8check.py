#!/usr/bin/env python3
"""d8check.py - F4-35: milestone D8's first and third QEMU acceptance tests, checked on a log.

    python3 d8check.py kernel.out

Test 1: every devicetree node whose compatible the kernel supports is bound by the generic
mechanism; the log lists bound and unbound nodes. Test 3: probing works when a consumer is
listed before its provider (at least one device was bound "after deferral").
The supported list is the set of compatibles this kernel's drivers declare (drivers.cc)."""
import re
import sys

SUPPORTED = {"arm,pl011", "arm,pl031", "arm,pl061", "virtio,mmio", "arm,cortex-a15-gic", "arm,armv8-timer",
             "fixed-clock", "arm,psci-1.0", "qemu,fw-cfg-mmio"}
rows = [l.split() for l in open(sys.argv[1]) if re.match(r"\s+(bound|FAILED|WAITING|no driver|disabled)\s", l)]
bad = 0
for r in rows:
    state = "no driver" if r[0] == "no" else r[0]
    path, compat = (r[2], r[3]) if r[0] == "no" else (r[1], r[2])
    if compat in SUPPORTED and state not in ("bound",):
        print("NOT BOUND: %s %s (%s)" % (path, compat, state))
        bad += 1
bound = sum(1 for r in rows if r[0] == "bound")
deferred = sum(1 for r in rows if "deferral)" in " ".join(r))
empty = re.search(r"\((\d+) FAILED virtio,mmio transports not listed", open(sys.argv[1]).read())
print("listed nodes: %d (bound %d, of which %d after a deferral); empty virtio transports: %s"
      % (len(rows), bound, deferred, empty.group(1) if empty else "0"))
print("test 1 (supported nodes bound by the generic mechanism, list printed): %s" % ("PASS" if bad == 0 and rows else "FAIL"))
print("test 3 (consumer listed before its provider still probes): %s" % ("PASS" if deferred > 0 else "FAIL"))
sys.exit(0 if bad == 0 and deferred > 0 else 1)
