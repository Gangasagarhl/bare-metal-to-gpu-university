# MP3: writes the SYNTHETIC raw benchmark records used to test mp3_report.cpp (Listing 4).
# Every time below is INVENTED by this seeded script. None was measured on any GPU.
#   mp3_report.in   = targets_example.txt (as signed at R1) + honest raw records
#   forensic_final.in = the same targets with two of them lowered after measuring
#                       + the same records, except that the attention reference
#                       was timed with a host wall clock (the forensic lab's evidence)
# Usage: python3 synthetic_raw.py <output folder>
import random
import sys

out = sys.argv[1] if len(sys.argv) > 1 else "."
rng = random.Random(2026)

ENV = [
    "env gpu SYNTHETIC-TEST-DATA",
    "env driver SYNTHETIC-TEST-DATA",
    "env toolkit SYNTHETIC-TEST-DATA",
    "env clocks not-locked:no-permission-on-the-shared-machine",
]

# kernel, size, precision, own base ms, reference base ms (invented)
CASES = [
    ("sgemm", 1024, "fp32", 1.25, 0.80),
    ("sgemm", 2048, "fp32", 8.90, 5.60),
    ("sgemm", 4096, "fp32", 82.0, 41.0),
    ("hgemm", 4096, "fp16", 20.0, 9.00),
    ("attention", 4096, "fp16", 3.00, 1.60),
]


def times(base, n=20, bimodal=False):
    v = []
    for i in range(n):
        t = base * (1.0 + rng.uniform(-0.02, 0.02))
        if bimodal and i % 3 == 0:          # some runs at a higher clock: unlocked clocks
            t = base * 0.86 * (1.0 + rng.uniform(-0.01, 0.01))
        v.append("%.3f" % t)
    return v


def records(attention_ref_timer):
    lines = ENV[:]
    for kernel, size, prec, own, ref in CASES:
        lines.append("check %s %d %s pass" % (kernel, size, prec))
        for impl, base in (("own", own), ("ref", ref)):
            timer = attention_ref_timer if (kernel == "attention" and impl == "ref") else "events"
            noisy = kernel == "sgemm" and size == 4096 and impl == "own"
            lines.append("runs %s %s %d %s %s warmup %.3f times %s" % (
                kernel, impl, size, prec, timer, base * 1.3, " ".join(times(base, bimodal=noisy))))
    return lines


targets = open(out + "/targets_example.txt").read().rstrip("\n")
honest = records("events")
rng.seed(2026)                               # identical times in both files
replayed = records("wallclock")
lowered = (targets.replace("target sgemm 4096 fp32 60", "target sgemm 4096 fp32 45")
           .replace("target hgemm 4096 fp16 50", "target hgemm 4096 fp16 40"))
banner = "# --- raw records written by the harness (SYNTHETIC: invented by synthetic_raw.py) ---"
with open(out + "/mp3_report.in", "w") as f:
    f.write(targets + "\n" + banner + "\n" + "\n".join(honest) + "\n")
with open(out + "/forensic_final.in", "w") as f:
    f.write(lowered + "\n" + banner + "\n" + "\n".join(replayed) + "\n")
print("wrote mp3_report.in and forensic_final.in (synthetic, seeded)")
