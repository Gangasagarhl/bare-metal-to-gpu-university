#!/usr/bin/env python3
"""Acceptance test C3 (1): run QEMU with the RTC following the host clock, and note the
host's own time at the moment the kernel's first "date:" line reaches the host. The kernel
reads the RTC just before printing that line, so the two times describe the same instant
to within the delay of one serial line.

usage: rtc_stamp.py TIMEOUT_SECONDS qemu-binary qemu-args..."""
import re
import subprocess
import sys
import time

p = subprocess.Popen(sys.argv[2:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
deadline = time.time() + float(sys.argv[1])
host = kernel = None
for line in p.stdout:
    line = line.rstrip("\r\n")
    print(line)
    m = re.match(r"date: .*\(unix (\d+)\)", line)
    if m and kernel is None:
        host, kernel = time.time(), int(m.group(1))
    if time.time() > deadline:
        p.kill()
        break
rc = p.wait()
print("== host ==")
if kernel is None:
    print("no 'date:' line from the kernel: FAIL")
    sys.exit(1)
diff = kernel - host
print("host clock when the first 'date:' line arrived: %.3f (unix seconds, UTC)" % host)
print("kernel's RTC reading in that line:               %d" % kernel)
print("difference kernel - host: %+.3f s (the RTC has whole seconds, so up to -1 s is expected)" % diff)
ok = abs(diff) <= 2.0
print("within 2 seconds: %s" % ("PASS" if ok else "FAIL"))
sys.exit(rc if ok else 1)
