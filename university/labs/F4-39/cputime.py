#!/usr/bin/env python3
"""cputime.py <command...> - run a command, pass its output through, then print how much
host CPU time it used (user + system, from the operating system's resource accounting of
child processes) next to the wall-clock time. Used to measure what an idle guest costs."""
import resource
import subprocess
import sys
import time

start = time.monotonic()
proc = subprocess.run(sys.argv[1:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
wall = time.monotonic() - start
use = resource.getrusage(resource.RUSAGE_CHILDREN)
sys.stdout.write(proc.stdout.decode("utf-8", "replace").replace("\r", ""))
print("host: wall %.2f s, guest process CPU %.2f s user + %.2f s system = %.0f %% of one CPU"
      % (wall, use.ru_utime, use.ru_stime, 100.0 * (use.ru_utime + use.ru_stime) / wall))
sys.exit(proc.returncode)
