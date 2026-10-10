// F7-17 cuda_wmma: the CUDA WMMA API (mma.h, installed in this build) for comparison.
// One warp computes one 16 x 16 tile of C = A * B; A row-major M x K, B column-major K x N.
// Build: nvcc -std=c++17 -O2 -arch=sm_80 -x cu cuda_wmma.cu.inc -o cuda_wmma
#include <mma.h>
#include <cuda_fp16.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace nvcuda;

static void check(cudaError_t e, const char* what)
{
    if (e != cudaSuccess) {
        std::printf("%s failed: %s (%s)\n", what, cudaGetErrorName(e), cudaGetErrorString(e));
        std::exit(1);
    }
}

__global__ void wmmaGemm(const half* A, const half* B, float* C, int M, int N, int K)
{
    const int tileRow = blockIdx.y * 16;
    const int tileCol = blockIdx.x * 16;
    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::col_major> b;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> acc;
    wmma::fill_fragment(acc, 0.0f);
    for (int k0 = 0; k0 < K; k0 += 16) {
        wmma::load_matrix_sync(a, A + tileRow * K + k0, K);
        wmma::load_matrix_sync(b, B + tileCol * K + k0, K);
        wmma::mma_sync(acc, a, b, acc);
    }
    wmma::store_matrix_sync(C + tileRow * N + tileCol, acc, N, wmma::mem_row_major);
}

int main()
{
    const int M = 64, N = 64, K = 64;
    std::vector<half> hA(M * K), hB(K * N);
    for (int i = 0; i < M * K; ++i) {
        hA[i] = __float2half(static_cast<float>(i % 7) / 8.0f);
        hB[i] = __float2half(static_cast<float>(i % 5) / 8.0f);
    }
    half* dA = nullptr;
    half* dB = nullptr;
    float* dC = nullptr;
    check(cudaMalloc(&dA, M * K * sizeof(half)), "cudaMalloc(dA)");
    check(cudaMalloc(&dB, K * N * sizeof(half)), "cudaMalloc(dB)");
    check(cudaMalloc(&dC, M * N * sizeof(float)), "cudaMalloc(dC)");
    check(cudaMemcpy(dA, hA.data(), M * K * sizeof(half), cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, hB.data(), K * N * sizeof(half), cudaMemcpyHostToDevice), "copy B");
    wmmaGemm<<<dim3(N / 16, M / 16), 32>>>(dA, dB, dC, M, N, K);
    check(cudaGetLastError(), "launch");
    check(cudaDeviceSynchronize(), "sync");
    std::vector<float> hC(M * N);
    check(cudaMemcpy(hC.data(), dC, M * N * sizeof(float), cudaMemcpyDeviceToHost), "copy C");
    int bad = 0;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            float ref = 0.0f;
            for (int k = 0; k < K; ++k) {
                ref += __half2float(hA[r * K + k]) * __half2float(hB[c * K + k]);
            }
            bad += (hC[r * N + c] != ref) ? 1 : 0;   // inputs are multiples of 1/8: sums are exact
        }
    }
    std::printf("wrong elements: %d\n", bad);
    return bad == 0 ? 0 : 1;
}
