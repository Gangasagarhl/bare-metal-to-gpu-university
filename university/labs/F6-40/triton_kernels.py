# F6-40 Listing 1: reduction, row softmax and a tiled matrix multiply written in Triton.
# NOT RUN in this build: Triton, PyTorch and a GPU are not installed in the build container.
# The file is only checked to be valid Python syntax (py_compile). Every Triton name used here
# (triton.jit, tl.program_id, tl.arange, tl.load, tl.store, tl.max, tl.sum, tl.exp, tl.dot,
# tl.zeros, tl.constexpr, triton.cdiv, the kernel[grid](...) launch) is written from memory and
# must be checked against the Triton documentation of the version you install.
import torch
import triton
import triton.language as tl


@triton.jit
def sum_kernel(x_ptr, partial_ptr, n, BLOCK: tl.constexpr):
    pid = tl.program_id(axis=0)                  # which block program am I
    offs = pid * BLOCK + tl.arange(0, BLOCK)     # a whole tile of indices at once
    mask = offs < n
    x = tl.load(x_ptr + offs, mask=mask, other=0.0)
    tl.store(partial_ptr + pid, tl.sum(x, axis=0))


@triton.jit
def softmax_kernel(y_ptr, x_ptr, row_stride, n_cols, BLOCK: tl.constexpr):
    row = tl.program_id(axis=0)                  # one program per row
    cols = tl.arange(0, BLOCK)                   # BLOCK is a power of two >= n_cols
    mask = cols < n_cols
    x = tl.load(x_ptr + row * row_stride + cols, mask=mask, other=-float("inf"))
    x = x - tl.max(x, axis=0)                    # safe softmax: subtract the row maximum
    num = tl.exp(x)
    den = tl.sum(num, axis=0)
    tl.store(y_ptr + row * row_stride + cols, num / den, mask=mask)


@triton.jit
def matmul_kernel(a_ptr, b_ptr, c_ptr, M, N, K,
                  stride_am, stride_ak, stride_bk, stride_bn, stride_cm, stride_cn,
                  BM: tl.constexpr, BN: tl.constexpr, BK: tl.constexpr):
    pid_m = tl.program_id(axis=0)
    pid_n = tl.program_id(axis=1)
    rm = pid_m * BM + tl.arange(0, BM)
    rn = pid_n * BN + tl.arange(0, BN)
    rk = tl.arange(0, BK)
    acc = tl.zeros((BM, BN), dtype=tl.float32)
    for k0 in range(0, K, BK):
        a = tl.load(a_ptr + rm[:, None] * stride_am + (k0 + rk)[None, :] * stride_ak,
                    mask=(rm[:, None] < M) & ((k0 + rk)[None, :] < K), other=0.0)
        b = tl.load(b_ptr + (k0 + rk)[:, None] * stride_bk + rn[None, :] * stride_bn,
                    mask=((k0 + rk)[:, None] < K) & (rn[None, :] < N), other=0.0)
        acc += tl.dot(a, b)
    tl.store(c_ptr + rm[:, None] * stride_cm + rn[None, :] * stride_cn, acc,
             mask=(rm[:, None] < M) & (rn[None, :] < N))


def softmax(x):
    rows, n_cols = x.shape
    y = torch.empty_like(x)
    softmax_kernel[(rows,)](y, x, x.stride(0), n_cols, BLOCK=triton.next_power_of_2(n_cols))
    return y


def matmul(a, b):
    M, K = a.shape
    _, N = b.shape
    c = torch.empty((M, N), device=a.device, dtype=torch.float32)
    grid = (triton.cdiv(M, 64), triton.cdiv(N, 64))
    matmul_kernel[grid](a, b, c, M, N, K, a.stride(0), a.stride(1), b.stride(0), b.stride(1),
                        c.stride(0), c.stride(1), BM=64, BN=64, BK=32)
    return c


if __name__ == "__main__":
    x = torch.randn(1000, 1500, device="cuda")
    print("softmax max error:", (softmax(x) - torch.softmax(x, dim=1)).abs().max().item())
    a = torch.randn(300, 200, device="cuda")
    b = torch.randn(200, 100, device="cuda")
    print("matmul max error:", (matmul(a, b) - a @ b).abs().max().item())
