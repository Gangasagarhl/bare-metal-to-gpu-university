#!/usr/bin/env python3
"""BR-08 Listing 4: explain the differences between two logs of the same controller.

    python3 -I compare_logs.py sim_run.csv kit_run.csv identified.txt [label_a label_b]

Reads two logs written by bench.hpp (same run record, same columns), measures the same
quantities in both, and points to the column of the kit log that explains each difference.
Writes the identified kit parameters to the third file, for the updated simulator (resim.cc).
Only numpy and the standard library are used.
"""
import sys
import numpy as np

T = 0.01  # s, the period written in both run records (checked below)


def load(path):
    head, rows = {}, []
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# ") and ": " in line:
                key, value = line[2:].split(": ", 1)
                head[key] = value
            elif line and not line.startswith("#") and not line.startswith("phase"):
                rows.append(line.split(","))
    phase = np.array([r[0] for r in rows])
    num = np.array([[float(x) for x in r[1:]] for r in rows])
    cols = dict(zip(["t_nom", "t", "set", "meas", "duty", "volts", "ref"], num.T))
    cols["phase"] = phase
    return head, cols


def hold(c, t0, t1):
    """Rows of the closed loop between t0 and t1 seconds after the loop started."""
    tl = c["t_nom"] - c["t_nom"][c["phase"] == "loop"][0]
    return (c["phase"] == "loop") & (tl >= t0 - 1e-9) & (tl < t1 - 1e-9)


def fit_step(c):
    """First order plus dead time, fitted to the period averages the encoder really gives."""
    m = c["phase"] == "step"
    t, y = c["t"][m], c["meas"][m]
    t0 = t[0]  # the duty was commanded right after this sample
    edges = np.concatenate(([t0], t))[:-1]  # each sample averages over (previous, this]
    best = None
    for L in np.arange(0.0, 0.0401, 0.0005):
        for tau in np.arange(0.05, 0.60, 0.002):
            def integ(x):
                u = np.maximum(x - t0 - L, 0.0)
                return u - tau * (1.0 - np.exp(-u / tau))
            shape = (integ(t) - integ(edges)) / np.maximum(t - edges, 1e-9)
            a = float(shape @ y) / float(shape @ shape)
            sse = float(np.sum((y - a * shape) ** 2))
            if best is None or sse < best[0]:
                best = (sse, a, tau, L)
    sse, a, tau, L = best
    return a, tau, L, np.sqrt(sse / len(y)), float(np.mean(c["volts"][m]))


def spectrum_peak(x):
    x = x - np.mean(x)
    amp = np.abs(np.fft.rfft(x * np.hanning(len(x)))) * 4.0 / len(x)
    freq = np.fft.rfftfreq(len(x), T)
    k = 2 + int(np.argmax(amp[2:]))  # skip the lowest bins (slow drift)
    return freq[k], amp[k]


def main():
    hs, s = load(sys.argv[1])
    hk, k = load(sys.argv[2])
    la, lb = (sys.argv[4], sys.argv[5]) if len(sys.argv) > 5 else ("sim", "kit")
    out = {}
    print("1. Run records")
    for key in ("conditions",):
        print("   %s: %s = %s | %s = %s" % (key, la, hs[key], lb, hk[key]))
    same = [key for key in hs if key not in ("world", "conditions")]
    diff = [key for key in same if hs[key] != hk.get(key)]
    print("   world:      %s = %s" % (la, hs["world"]))
    print("               %s = %s" % (lb, hk["world"]))
    print("   identical in both records: %s" % ", ".join(k2 for k2 in same if k2 not in diff))
    print("   different (other than world and conditions): %s" % (", ".join(diff) or "none"))
    assert hs["period_s"].startswith("0.01"), "period differs from T"

    print("\n2. Timing (t_sample column)")
    for name, c in ((la, s), (lb, k)):
        dt = np.diff(c["t"]) * 1000
        print("   %s: interval mean %.3f ms, min %.2f, max %.2f, std %.3f ms" %
              (name, dt.mean(), dt.min(), dt.max(), dt.std()))
    late = (k["t"] - k["t_nom"]) * 1000
    out["lateness_max_ms"] = float(late.max())
    print("   %s: each sample is 0 to %.2f ms after its nominal time" % (lb, late.max()))

    print("\n3. At rest (phase rest, duty 0)")
    for name, c in ((la, s), (lb, k)):
        r = c["phase"] == "rest"
        values = ", ".join("%.2f" % v for v in sorted(set(np.round(c["meas"][r], 2))))
        print("   %s: measured std %.2f rad/s, values {%s}, supply %.2f V" %
              (name, c["meas"][r].std(), values, c["volts"][r].mean()))
    out["supply_rest_v"] = float(k["volts"][k["phase"] == "rest"].mean())
    out["rest_noise"] = float(k["meas"][k["phase"] == "rest"].std())

    print("\n4. Open-loop step test (duty 0.25), fitted: final speed, time constant, dead time")
    for name, c in ((la, s), (lb, k)):
        a, tau, L, rms, v = fit_step(c)
        print("   %s: final %.1f rad/s, tau %.3f s, dead time %.1f ms, fit rms %.2f rad/s,"
              " mean supply %.2f V" % (name, a, tau, L * 1000, rms, v))
        if name == lb:
            out.update(step_final=a, tau=tau, dead_time=L, step_volts=v)
    m = k["phase"] == "step"
    print("   %s supply during the step: %.2f V at the start, %.2f V at the end" %
          (lb, k["volts"][m][1], k["volts"][m][-1]))

    print("\n5. Calibration: the robot's measurement against the external reference")
    ratios = []
    for name, c in ((la, s), (lb, k)):
        line = []
        for t0, t1, sp in ((0.8, 1.2, 150), (2.0, 2.4, 250)):
            h = hold(c, t0, t1)
            ratio = c["meas"][h].mean() / c["ref"][h].mean()
            line.append("hold %d: measured %.2f, reference %.2f, ratio %.4f" %
                        (sp, c["meas"][h].mean(), c["ref"][h].mean(), ratio))
            if name == lb:
                ratios.append(ratio)
        print("   %s: %s" % (name, "; ".join(line)))
    ratio = float(np.mean(ratios))
    out["cpr_true_est"] = 1024.0 * ratio
    print("   %s: measured/reference = %.4f -> encoder counts per revolution about %.0f, "
          "not 1024" % (lb, ratio, 1024.0 * ratio))

    print("\n6. Closed loop (phase loop)")
    for name, c in ((la, s), (lb, k)):
        for t0, t1, sp in ((0.0, 1.2, 150), (1.2, 2.4, 250)):
            h, last = hold(c, t0, t1), hold(c, t1 - 0.4, t1)
            over = 100 * (c["meas"][h].max() - sp) / sp
            final = c["ref"][last].mean()
            over_ref = 100 * (c["ref"][h].max() - final) / final
            print("   %s %d: overshoot %5.1f %% (reference: %4.1f %% above its final %.1f);"
                  " last 0.4 s: error std %.2f rad/s, mean duty %.3f, supply %.2f V" %
                  (name, sp, over, over_ref, final, (c["meas"][last] - sp).std(),
                   c["duty"][last].mean(), c["volts"][last].mean()))
    # static identification from the two holds, with the true (reference) speeds
    w, volts = [], []
    for t0, t1 in ((0.8, 1.2), (2.0, 2.4)):
        h = hold(k, t0, t1)
        w.append(k["ref"][h].mean())
        volts.append((k["duty"][h] * k["volts"][h]).mean())
    kv = (w[1] - w[0]) / (volts[1] - volts[0])
    dead = volts[0] - w[0] / kv
    out.update(gain_per_volt=kv, deadband_v=dead)
    print("   %s, from the two holds (reference speed against duty x supply): "
          "%.1f rad/s per volt above a %.2f V deadband" % (lb, kv, dead))

    print("\n7. Where the %s log's speed noise comes from (holds, last 0.8 s)" % lb)
    for t0, t1 in ((0.4, 1.2), (1.6, 2.4)):
        h = np.where(hold(k, t0, t1))[0]
        dt = k["t"][h] - k["t"][h - 1]
        err = k["meas"][h] - ratio * k["ref"][h]
        pred = ratio * k["ref"][h] * (dt / T - 1.0)
        cc = np.corrcoef(err, pred)[0, 1]
        print("   hold at %.0f: measured-minus-reference std %.2f rad/s; correlation with the "
              "interval error %.2f" % (k["set"][h].mean(), err.std(), cc))

    print("\n8. Vibration: spectrum of the reference speed in the holds (last 0.8 s)")
    for name, c in ((la, s), (lb, k)):
        for t0, t1 in ((0.4, 1.2), (1.6, 2.4)):
            h = hold(c, t0, t1)
            f, a = spectrum_peak(c["ref"][h])
            rev = c["ref"][h].mean() / (2 * np.pi)
            print("   %s hold %.0f: largest peak %.1f Hz, amplitude %.2f rad/s; "
                  "one revolution = %.1f Hz" % (name, c["set"][h].mean(), f, a, rev))
            if name == lb and t0 > 1.0:
                # a voltage-equivalent once-per-revolution disturbance for the simulator
                wr = 2 * np.pi * f
                out["ripple_v"] = a * np.sqrt(1 + (wr * out["tau"]) ** 2) / kv

    with open(sys.argv[3], "w") as f:
        for key in sorted(out):
            f.write("%s %.6g\n" % (key, out[key]))
    print("\nidentified kit parameters written to %s:" % sys.argv[3])
    for key in sorted(out):
        print("   %-16s %.4g" % (key, out[key]))


if __name__ == "__main__":
    main()
