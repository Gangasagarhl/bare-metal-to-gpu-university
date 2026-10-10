// F1-61 Listing 1: one warp multiplies two 16x16 FP16 tiles with the WMMA API,
// accumulating in FP32, and the host checks the result. Untested on hardware.
// Build for a GPU with tensor cores, e.g. nvcc -arch=sm_80 (run.sh does); the lab
// runner's default target has none, so that build only keeps the trap below.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_fp16.h>
#include <cuda_runtime.h>
#include <mma.h>

__global__ void tile16(const half* a, const half* b, float* c)
{
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ < 700
    __trap();                                    // built for a GPU without tensor cores: stop loudly
#else
    using namespace nvcuda;
    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> fa;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::col_major> fb;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> fc;
    wmma::fill_fragment(fc, 0.0f);
    wmma::load_matrix_sync(fa, a, 16);           // the whole warp loads the tile together
    wmma::load_matrix_sync(fb, b, 16);
    wmma::mma_sync(fc, fa, fb, fc);              // c = a * b + c on the matrix unit
    wmma::store_matrix_sync(c, fc, 16, wmma::mem_row_major);
#endif
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 16;
    std::vector<half> hA(n * n), hB(n * n);
    std::vector<float> hC(n * n), ref(n * n, 0.0f);
    for (int i = 0; i < n * n; ++i) {
        hA[i] = __float2half(static_cast<float>(i % 7) * 0.25f);
        hB[i] = __float2half(static_cast<float>(i % 5) * 0.5f);
    }
    for (int r = 0; r < n; ++r) {                // B is stored column-major: B(k, c) = hB[c * n + k]
        for (int c = 0; c < n; ++c) {
            for (int k = 0; k < n; ++k) {
                ref[r * n + c] += __half2float(hA[r * n + k]) * __half2float(hB[c * n + k]);
            }
        }
    }
    half* dA = nullptr;
    half* dB = nullptr;
    float* dC = nullptr;
    check(cudaMalloc(&dA, n * n * sizeof(half)), "cudaMalloc a");
    check(cudaMalloc(&dB, n * n * sizeof(half)), "cudaMalloc b");
    check(cudaMalloc(&dC, n * n * sizeof(float)), "cudaMalloc c");
    check(cudaMemcpy(dA, hA.data(), n * n * sizeof(half), cudaMemcpyHostToDevice), "copy a");
    check(cudaMemcpy(dB, hB.data(), n * n * sizeof(half), cudaMemcpyHostToDevice), "copy b");
    tile16<<<1, 32>>>(dA, dB, dC);               // one warp: 32 threads on NVIDIA GPUs
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hC.data(), dC, n * n * sizeof(float), cudaMemcpyDeviceToHost), "copy c");
    int errors = 0;
    for (int i = 0; i < n * n; ++i) {
        if (hC[i] != ref[i]) { ++errors; }
    }
    std::printf("checked %d elements, %d errors\n", n * n, errors);
    check(cudaFree(dA), "cudaFree a");
    check(cudaFree(dB), "cudaFree b");
    check(cudaFree(dC), "cudaFree c");
    return errors == 0 ? 0 : 1;
}
