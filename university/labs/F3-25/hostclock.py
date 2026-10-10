#!/usr/bin/env python3
"""F3-25: run QEMU with the serial port on a pipe and stamp every line with the host's
monotonic clock as it arrives. Then compare the kernel's 10 s sleep with the host's view.

usage: hostclock.py TOLERANCE_PERCENT qemu-binary qemu-args...   (exit code: QEMU's)
"""
import re
import subprocess
import sys
import time

tol = float(sys.argv[1])
p = subprocess.Popen(sys.argv[2:] + ["-serial", "stdio"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
t0 = time.monotonic()
start = end = None
kernel_s = None
for raw in p.stdout:
    now = time.monotonic() - t0
    line = raw.decode(errors="replace").rstrip("\n")
    print("[host %8.3f] %s" % (now, line), flush=True)
    if "sleep: start" in line:
        start = now
    m = re.search(r"sleep: end, the kernel clock measured (\d+\.\d+) s", line)
    if m:
        end, kernel_s = now, float(m.group(1))
rc = p.wait()
if start is None or end is None:
    print("hostclock: the sleep markers were not seen")
    sys.exit(rc if rc != 0 else 1)
host_s = end - start
err = (kernel_s - host_s) / host_s * 100.0
print("hostclock: requested 10.000 s; kernel measured %.6f s; host measured %.3f s between the two lines" % (kernel_s, host_s))
print("hostclock: kernel vs host differ by %+.2f %% (tolerance %.1f %%): %s"
      % (err, tol, "within tolerance" if abs(err) <= tol else "OUTSIDE tolerance"))
sys.exit(rc)
