// F7-19 Listing 2: the vendor library as the reference: cuBLAS SGEMM (installed in this build).
// rocBLAS follows the same BLAS conventions; its calls are in the chapter's unverified box.
// Build: nvcc -std=c++17 -O2 -x cu cublas_ref.cu.inc -lcublas -o cublas_ref
#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cudaError_t e, const char* what)
{
    if (e != cudaSuccess) {
        std::printf("%s failed: %s (%s)\n", what, cudaGetErrorName(e), cudaGetErrorString(e));
        std::exit(1);
    }
}

static void checkBlas(cublasStatus_t s, const char* what)
{
    if (s != CUBLAS_STATUS_SUCCESS) {
        std::printf("%s failed: cuBLAS status %d\n", what, static_cast<int>(s));
        std::exit(1);
    }
}

int main()
{
    const int M = 1024, N = 512, K = 256;
    std::vector<float> hA(static_cast<size_t>(M) * K, 0.5f), hB(static_cast<size_t>(K) * N, 0.25f);
    float *dA = nullptr, *dB = nullptr, *dC = nullptr;
    check(cudaMalloc(&dA, hA.size() * sizeof(float)), "cudaMalloc(dA)");
    check(cudaMalloc(&dB, hB.size() * sizeof(float)), "cudaMalloc(dB)");
    check(cudaMalloc(&dC, static_cast<size_t>(M) * N * sizeof(float)), "cudaMalloc(dC)");
    check(cudaMemcpy(dA, hA.data(), hA.size() * sizeof(float), cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, hB.data(), hB.size() * sizeof(float), cudaMemcpyHostToDevice), "copy B");
    cublasHandle_t handle;
    checkBlas(cublasCreate(&handle), "cublasCreate");          // once, outside any timed region
    const float alpha = 1.0f, beta = 0.0f;
    // row-major C = A * B computed as column-major C^T = B^T * A^T (Listing 1)
    checkBlas(cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, N, M, K, &alpha, dB, N, dA, K, &beta, dC, N),
              "cublasSgemm");
    check(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
    std::vector<float> hC(static_cast<size_t>(M) * N);
    check(cudaMemcpy(hC.data(), dC, hC.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy C");
    int bad = 0;
    for (float v : hC) {
        bad += (v != 0.5f * 0.25f * K);                       // exact: every term is 1/8
    }
    std::printf("wrong elements: %d\n", bad);
    checkBlas(cublasDestroy(handle), "cublasDestroy");
    check(cudaFree(dA), "cudaFree");
    check(cudaFree(dB), "cudaFree");
    check(cudaFree(dC), "cudaFree");
    return bad == 0 ? 0 : 1;
}
