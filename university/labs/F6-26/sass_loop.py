# F6-26: print the body of the main loop of one kernel's SASS (from the target of the last
# backward branch to that branch), keeping loads, tensor-core instructions and branches.
# Input: "offset  instruction" lines (cuobjdump -sass, reformatted by run.sh) on stdin.
import re
import sys

lines = []
for raw in sys.stdin:
    m = re.match(r"([0-9a-f]{4,})\s+(.*)", raw.strip())
    if m:
        lines.append((int(m.group(1), 16), m.group(2)))
loop = None
for off, ins in lines:
    b = re.search(r"BRA 0x([0-9a-f]+)", ins)
    if b and int(b.group(1), 16) < off:
        loop = (int(b.group(1), 16), off)
if loop is None:
    print("no backward branch found")
    sys.exit(1)
print("loop body: 0x%04x to 0x%04x (%d instructions)" % (loop[0], loop[1],
      sum(1 for off, _ in lines if loop[0] <= off <= loop[1])))
for off, ins in lines:
    if loop[0] <= off <= loop[1] and re.search(r"HMMA|LD|BRA", ins):
        print("%04x  %s" % (off, ins))
