# median.py: reads lines "ms: make A  parse B  sort C" from standard input and prints
# the median of each step over all runs (used by run.sh; numbers are measurements).
import re
import statistics
import sys

steps = {}
for line in sys.stdin:
    if not line.startswith("ms:"):
        continue
    for name, value in re.findall(r"(\w+) ([0-9.]+)", line.split("ms:", 1)[-1]):
        steps.setdefault(name, []).append(float(value))
for name, values in steps.items():
    print(f"{name:6s} runs {len(values)}  median {statistics.median(values):6.1f} ms  "
          f"min {min(values):6.1f}  max {max(values):6.1f}")
