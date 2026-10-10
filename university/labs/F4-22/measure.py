#!/usr/bin/env python3
"""measure.py - run one command and report how much host CPU time it used against wall time.
QEMU with TCG runs the guest CPU on a host thread: while the guest halts, that thread sleeps;
while the guest polls, it runs. So host CPU time is a stand-in for "was the CPU busy". It is
not a power measurement. Usage: measure.py <label> <command...>"""
import resource
import subprocess
import sys
import time

before = resource.getrusage(resource.RUSAGE_CHILDREN)
t0 = time.monotonic()
proc = subprocess.run(sys.argv[2:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
wall = time.monotonic() - t0
after = resource.getrusage(resource.RUSAGE_CHILDREN)
cpu = (after.ru_utime - before.ru_utime) + (after.ru_stime - before.ru_stime)
sys.stdout.write(proc.stdout.replace("\r", ""))
print("%s: QEMU exit %d, wall %.2f s, host CPU %.2f s, CPU/wall %.0f %%"
      % (sys.argv[1], proc.returncode, wall, cpu, 100 * cpu / wall if wall else 0))
sys.exit(proc.returncode)
