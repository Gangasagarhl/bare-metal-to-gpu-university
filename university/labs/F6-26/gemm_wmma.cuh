// F6-26 Listing 1: mixed-precision GEMM on tensor cores with the WMMA API (mma.h).
// C (FP32, M x N) = A (T, M x K) * B (T, K x N), all row-major, T = half or __nv_bfloat16.
// A block of 4 warps (2 x 2) computes a 64 x 64 tile of C; each warp owns a 32 x 32 tile,
// held as 2 x 2 accumulator fragments of 16 x 16, and walks K in steps of 16.
// Requires M, N, K to be multiples of 16 (the launcher checks). Needs compute capability
// 8.0 or newer for BF16 (7.0 for FP16): build with -arch=sm_80; a build for an older target
// keeps only the trap below, so that a wrong build fails loudly instead of returning zeros.
#pragma once
#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <mma.h>

constexpr int WM = 16, WN = 16, WK = 16;           // one WMMA operation: 16 x 16 x 16
constexpr int WARP_TILE = 32;                       // 2 x 2 fragments per warp
constexpr int BLOCK_TILE = 64;                      // 2 x 2 warps per block

template <typename T>
__global__ void gemmWmma(const T* A, const T* B, float* C, int M, int N, int K)
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 800
    __trap();                                       // built for a target without BF16 WMMA
#else
    using namespace nvcuda;
    const int warp = threadIdx.x / warpSize;        // 0..3
    const int warpRow = blockIdx.y * BLOCK_TILE + (warp / 2) * WARP_TILE;
    const int warpCol = blockIdx.x * BLOCK_TILE + (warp % 2) * WARP_TILE;
    if (warpRow >= M || warpCol >= N) {
        return;                                     // the whole warp leaves together
    }
    wmma::fragment<wmma::accumulator, WM, WN, WK, float> acc[2][2];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            wmma::fill_fragment(acc[i][j], 0.0f);
        }
    }
    wmma::fragment<wmma::matrix_a, WM, WN, WK, T, wmma::row_major> a[2];
    wmma::fragment<wmma::matrix_b, WM, WN, WK, T, wmma::row_major> b[2];
    for (int k = 0; k < K; k += WK) {
        for (int i = 0; i < 2; ++i) {
            const int r = warpRow + i * WM;
            if (r < M) {
                wmma::load_matrix_sync(a[i], A + r * K + k, K);   // leading dimension K
            }
        }
        for (int j = 0; j < 2; ++j) {
            const int c = warpCol + j * WN;
            if (c < N) {
                wmma::load_matrix_sync(b[j], B + k * N + c, N);   // leading dimension N
            }
        }
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                if (warpRow + i * WM < M && warpCol + j * WN < N) {
                    wmma::mma_sync(acc[i][j], a[i], b[j], acc[i][j]);   // acc += a * b
                }
            }
        }
    }
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            const int r = warpRow + i * WM, c = warpCol + j * WN;
            if (r < M && c < N) {
                wmma::store_matrix_sync(C + r * N + c, acc[i][j], N, wmma::mem_row_major);
            }
        }
    }
#endif
}
