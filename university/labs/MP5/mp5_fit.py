#!/usr/bin/env python3
# MP5 starter, Listing 6: the alpha-beta model fitted to a bandwidth curve (milestone 1 part of
# the "model-versus-measurement report"). Usage: python3 mp5_fit.py bench.out
# Model per rank count N (F8-03, extended for chunks): t = m * alpha + b / beta, where
#   m = messages each rank sends = 2(N-1) * chunks per segment   (column "msgs")
#   b = bytes each rank sends    = 2(N-1)/N * S                  (column "bytes_sent")
# alpha and 1/beta are found by least squares on RELATIVE error (each row divided by the
# measured time), so 4-byte and 64-MiB points count equally. Then every point is predicted
# and the ratio predicted / measured is printed: the model-accuracy evidence of the rubric.
import sys

import numpy as np


def main(path):
    rows = []
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            p = line.split()
            if len(p) < 5 or p[2] == "FAILED":
                continue
            rows.append((int(p[0]), int(p[1]), int(p[2]), float(p[3]), float(p[4])))
    worst_all = 0.0
    for n in sorted({r[0] for r in rows}):
        sel = [r for r in rows if r[0] == n]
        m = np.array([r[2] for r in sel], dtype=float)
        b = np.array([r[3] for r in sel], dtype=float)
        t = np.array([r[4] for r in sel], dtype=float)
        a = np.column_stack([m / t, b / t])            # unknowns: alpha_us, us_per_byte
        coef, *_ = np.linalg.lstsq(a, np.ones_like(t), rcond=None)
        coef = np.maximum(coef, 0.0)                   # a negative cost has no meaning
        alpha_us, us_per_byte = coef
        beta = 1.0 / (us_per_byte * 1e3) if us_per_byte > 0 else float("inf")   # GB/s
        pred = m * alpha_us + b * us_per_byte
        ratio = pred / t
        err = np.abs(ratio - 1.0)
        worst_all = max(worst_all, float(err.max()))
        print(f"N = {n}: fitted alpha = {alpha_us:.2f} us per message, beta = {beta:.3f} GB/s per rank")
        print(f"  {'bytes':>10} {'measured_us':>12} {'model_us':>12} {'model/meas':>11}")
        for r, p_, q in zip(sel, pred, ratio):
            flag = "  <- outside +-50 %" if abs(q - 1.0) > 0.5 else ""
            print(f"  {r[1]:>10} {r[4]:>12.2f} {p_:>12.2f} {q:>11.2f}{flag}")
        print(f"  median |model/meas - 1| = {np.median(err):.2f}; largest = {err.max():.2f}; "
              f"points outside +-50 %: {int((err > 0.5).sum())} of {len(err)}")
    print("measured on the build container (CPU threads, shared machine): these alpha and beta describe this run, "
          "not any GPU or network")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "bench.out"))
