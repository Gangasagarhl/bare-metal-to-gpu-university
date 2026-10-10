# F6-40 Listing 2: a NumPy model of the block-program style, run in this build.
# A tiny stand-in "tl" gives each program a whole tile (a NumPy array) of indices, masked loads
# with a fill value, and reductions over the tile. The three kernels have the same structure as
# Listing 1; a loop over program ids plays the role of the GPU's grid. It is the university's
# own teaching model of the IDEA, not Triton: no compiler, no GPU, no performance.
import sys

import numpy as np


class TileLang:
    """The few block-level operations the kernels need, with NumPy arrays as tiles."""

    @staticmethod
    def arange(start, stop):
        return np.arange(start, stop)

    @staticmethod
    def load(buf, offs, mask, other):
        safe = np.where(mask, offs, 0)           # never index outside the buffer
        return np.where(mask, buf[safe], np.float32(other)).astype(np.float32)

    @staticmethod
    def store(buf, offs, value, mask):
        buf[offs[mask]] = value[mask]


tl = TileLang()


def sum_kernel(pid, x, partial, n, BLOCK):
    offs = pid * BLOCK + tl.arange(0, BLOCK)
    mask = offs < n
    v = tl.load(x, offs, mask, 0.0)
    partial[pid] = v.sum(dtype=np.float32)


def softmax_kernel(row, y, x, row_stride, n_cols, BLOCK, fill):
    cols = tl.arange(0, BLOCK)
    mask = cols < n_cols
    v = tl.load(x, row * row_stride + cols, mask, fill)
    v = v - v.max()
    num = np.exp(v)
    den = num.sum(dtype=np.float32)
    tl.store(y, row * row_stride + cols, num / den, mask)


def matmul_kernel(pid_m, pid_n, a, b, c, M, N, K, BM, BN, BK):
    rm = pid_m * BM + tl.arange(0, BM)
    rn = pid_n * BN + tl.arange(0, BN)
    rk = tl.arange(0, BK)
    acc = np.zeros((BM, BN), dtype=np.float32)
    for k0 in range(0, K, BK):
        ka = k0 + rk
        ta = tl.load(a, rm[:, None] * K + ka[None, :], (rm[:, None] < M) & (ka[None, :] < K), 0.0)
        tb = tl.load(b, ka[:, None] * N + rn[None, :], (ka[:, None] < K) & (rn[None, :] < N), 0.0)
        acc += ta @ tb
    tl.store(c, rm[:, None] * N + rn[None, :], acc, (rm[:, None] < M) & (rn[None, :] < N))


def next_pow2(n):
    p = 1
    while p < n:
        p *= 2
    return p


def run_softmax(x2d, fill):
    rows, n_cols = x2d.shape
    x = x2d.reshape(-1).astype(np.float32)
    y = np.zeros_like(x)
    for row in range(rows):                       # the grid: one program per row
        softmax_kernel(row, y, x, n_cols, n_cols, next_pow2(n_cols), fill)
    return y.reshape(rows, n_cols)


def ref_softmax(x2d):
    z = x2d.astype(np.float64)
    z = np.exp(z - z.max(axis=1, keepdims=True))
    return z / z.sum(axis=1, keepdims=True)


def main():
    rng = np.random.default_rng(7)
    # reduction
    n, block = 100_003, 1024
    x = rng.standard_normal(n).astype(np.float32)
    partial = np.zeros((n + block - 1) // block, dtype=np.float32)
    for pid in range(partial.size):
        sum_kernel(pid, x, partial, n, block)
    print(f"sum: {partial.size} programs of {block}; |model - float64 sum| = "
          f"{abs(float(partial.sum(dtype=np.float64)) - float(x.sum(dtype=np.float64))):.3g}")
    # softmax, with a column count that is not a power of two
    for rows, cols, shift in [(64, 1000, 0.0), (64, 1024, 0.0)]:
        xs = (rng.standard_normal((rows, cols)) * 3.0 + shift).astype(np.float32)
        err = np.abs(run_softmax(xs, -np.inf) - ref_softmax(xs)).max()
        print(f"softmax {rows} x {cols} (BLOCK {next_pow2(cols)}): max error {err:.3g}")
    # matmul with sizes that are not multiples of the tiles
    M, N, K, BM, BN, BK = 300, 100, 200, 64, 64, 32
    a = rng.standard_normal((M, K)).astype(np.float32)
    b = rng.standard_normal((K, N)).astype(np.float32)
    c = np.zeros(M * N, dtype=np.float32)
    grid = ((M + BM - 1) // BM, (N + BN - 1) // BN)
    for pm in range(grid[0]):
        for pn in range(grid[1]):
            matmul_kernel(pm, pn, a.reshape(-1), b.reshape(-1), c, M, N, K, BM, BN, BK)
    ref = a.astype(np.float64) @ b.astype(np.float64)
    print(f"matmul {M}x{N}x{K}, grid {grid[0]}x{grid[1]} of {BM}x{BN} tiles: "
          f"max error {np.abs(c.reshape(M, N) - ref).max():.3g}")


def forensic():
    rng = np.random.default_rng(11)
    print("softmax with the masked lanes filled with 0.0 instead of -inf")
    for rows, cols, shift in [(64, 1024, 0.0), (64, 1000, 0.0), (64, 1000, -30.0), (64, 1000, -120.0)]:
        xs = (rng.standard_normal((rows, cols)) * 3.0 + shift).astype(np.float32)
        y = run_softmax(xs, 0.0)
        err = np.abs(y - ref_softmax(xs)).max()
        print(f"  {rows} x {cols}, values around {shift:7.1f}, BLOCK {next_pow2(cols)}: "
              f"max error {err:.3g}, worst row sum {y.sum(axis=1).min():.3g}")


if __name__ == "__main__":
    print("numpy", np.__version__)
    if len(sys.argv) > 1 and sys.argv[1] == "--forensic":
        forensic()
    else:
        main()
