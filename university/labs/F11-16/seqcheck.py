# F11-16 Listing 4 (answer-key tool): match every frame the drone accepted with a frame
# the ground station sent, and report what does not match.
import csv
import sys

rx = list(csv.DictReader(open(sys.argv[1])))
tx = list(csv.DictReader(open(sys.argv[2])))
sent = {}
for r in tx:
    sent.setdefault((int(r["seq"]), r["msg"]), []).append(int(r["t_ms"]))

matched, unmatched = [], []
for r in rx:
    t = int(r["t_ms"])
    times = sent.get((int(r["seq"]), r["msg"]), [])
    if any(0 <= t - s <= 50 for s in times):
        matched.append(r)
    else:
        unmatched.append(r)

print("frames accepted by the drone:", len(rx))
print("matched to a ground-station send (same seq and message, 0-50 ms earlier):", len(matched))
print("NOT sent by the ground station:", len(unmatched))
for r in unmatched:
    print("   t_ms %s seq %s %s rssi %s" % (r["t_ms"], r["seq"], r["msg"], r["rssi_dbm"]))

seen = {}
dups = 0
for r in rx:
    k = int(r["seq"])
    if k in seen and int(r["t_ms"]) - seen[k] < 10000:
        dups += 1
        print("duplicate seq %d at t_ms %s (first at t_ms %d)" % (k, r["t_ms"], seen[k]))
    seen[k] = int(r["t_ms"])
print("duplicate sequence numbers within 10 s:", dups)

def stats(rows):
    v = sorted(int(r["rssi_dbm"]) for r in rows)
    return "min %d, median %d, max %d dBm" % (v[0], v[len(v) // 2], v[-1])

near = [r for r in matched if 40000 <= int(r["t_ms"]) <= 52000]
print("rssi of matched frames 40-52 s:", stats(near))
print("rssi of unmatched frames:      ", stats(unmatched))
