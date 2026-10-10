#!/usr/bin/env python3
"""analyse.py - F10-32 Listing 4: a first-pass flight review of a UDF log with numpy.

It answers the questions an engineer asks first, in this order: what did the vehicle think it
was doing (modes and messages), was the energy fine (battery), was the attitude sane, were the
sensors fine (GNSS satellites and fix), and when did the estimator lose its position.
The thresholds marked "exercise" are the university's choices, not a flight stack's.

usage: analyse.py LOG
"""
import sys

import numpy as np

from dfread import Log

MODES = {0: "HOLD", 1: "GUIDED", 2: "AUTO", 3: "LAND"}
REASONS = {0: "command", 1: "mission end", 2: "failsafe: GPS", 3: "failsafe: battery"}


def column(rows, key):
    return np.array([r[key] for r in rows], dtype=float)


def main(path):
    log = Log(path)
    print(f"== {path.split('/')[-1]}: {len(log.records)} records")
    params = {r["Name"]: r["Value"] for r in log.of("PARM")}
    print("parameters:", ", ".join(f"{k}={v:g}" for k, v in params.items()))

    print("-- timeline (MODE, MSG, ERR)")
    events = [(r["TimeUS"], f"MODE {MODES[r['Mode']]} ({REASONS[r['Reason']]})") for r in log.of("MODE")]
    events += [(r["TimeUS"], f"MSG  {r['Message']}") for r in log.of("MSG")]
    events += [(r["TimeUS"], f"ERR  subsystem {r['Subsys']} code {r['Code']}") for r in log.of("ERR")]
    for t, what in sorted(events):
        print(f"  {t / 1e6:6.2f} s  {what}")

    bat = log.of("BAT")
    v, i, tb = column(bat, "Volt"), column(bat, "Curr"), column(bat, "TimeUS") / 1e6
    k = int(np.argmin(v))
    low = params.get("U_BATT_LOW", 0)
    print(f"-- battery: min {v[k]:.2f} V at {tb[k]:.0f} s (current {i[k]:.1f} A); "
          f"low threshold {low:g} V -> {'BELOW' if v[k] < low else 'never reached'}")

    att = log.of("ATT")
    roll, pitch = column(att, "Roll"), column(att, "Pitch")
    print(f"-- attitude: max |roll| {np.max(np.abs(roll)):.1f} deg, "
          f"max |pitch| {np.max(np.abs(pitch)):.1f} deg (exercise limit 30 deg)")

    gps = log.of("GPS")
    sats, status, tg = column(gps, "NSats"), column(gps, "Status"), column(gps, "TimeUS") / 1e6
    bad = tg[status < 3]
    print(f"-- GNSS: satellites min {sats.min():.0f}, median {np.median(sats):.0f}; "
          f"samples without 3D fix: {len(bad)}"
          + (f" (from {bad[0]:.1f} s to {bad[-1]:.1f} s)" if len(bad) else ""))
    minsats = params.get("U_GPS_MINSATS", 0)
    first_low = tg[sats < minsats]
    if len(first_low):
        print(f"   first sample below U_GPS_MINSATS={minsats:g}: {first_low[0]:.1f} s")

    pos = log.of("POS")
    ok, tp = column(pos, "PosOK"), column(pos, "TimeUS") / 1e6
    lost = tp[ok == 0]
    if len(lost):
        x, y = column(pos, "X"), column(pos, "Y")
        drift = np.hypot(x[-1] - x[ok == 0][0], y[-1] - y[ok == 0][0])
        print(f"-- estimator: position lost from {lost[0]:.1f} s; the vehicle then moved "
              f"{drift:.1f} m before the log ends, with nothing correcting the drift")
    else:
        print("-- estimator: position valid for the whole log")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
