// F6-35 Listing 5: C = A * B for row-major matrices with cuBLAS SGEMM, using the operand swap of
// Listing 1 (cuBLAS is column-major). Every cuBLAS call returns a cublasStatus_t that is checked.
// Build: nvcc -std=c++17 -O2 sgemm_cublas.cu -lcublas -o sgemm_cublas
// Untested on hardware: the build container has no GPU.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cublas_v2.h>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

static void checkBlas(cublasStatus_t st, const char* what)
{
    if (st != CUBLAS_STATUS_SUCCESS) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cublasGetStatusName(st), cublasGetStatusString(st));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int m = 300;   // rows of A and C
    const int k = 200;   // columns of A, rows of B
    const int n = 100;   // columns of B and C
    std::vector<float> hA(static_cast<std::size_t>(m) * k);
    std::vector<float> hB(static_cast<std::size_t>(k) * n);
    for (std::size_t i = 0; i < hA.size(); ++i) {
        hA[i] = static_cast<float>(static_cast<int>(i % 7) - 3);
    }
    for (std::size_t i = 0; i < hB.size(); ++i) {
        hB[i] = static_cast<float>(static_cast<int>(i % 5) - 2);
    }
    cublasHandle_t handle = nullptr;
    checkBlas(cublasCreate(&handle), "cublasCreate");
    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    check(cudaMalloc(&dA, hA.size() * sizeof(float)), "cudaMalloc A");
    check(cudaMalloc(&dB, hB.size() * sizeof(float)), "cudaMalloc B");
    check(cudaMalloc(&dC, static_cast<std::size_t>(m) * n * sizeof(float)), "cudaMalloc C");
    check(cudaMemcpy(dA, hA.data(), hA.size() * sizeof(float), cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, hB.data(), hB.size() * sizeof(float), cudaMemcpyHostToDevice), "copy B");
    const float alpha = 1.0f;
    const float beta = 0.0f;
    // row-major C (m x n) = A * B  <=>  column-major C^T (n x m) = B^T * A^T
    checkBlas(cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, n, m, k, &alpha, dB, n, dA, k, &beta, dC, n),
              "cublasSgemm");
    std::vector<float> hC(static_cast<std::size_t>(m) * n);
    check(cudaMemcpy(hC.data(), dC, hC.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy C");
    int wrong = 0;
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            float ref = 0.0f;
            for (int p = 0; p < k; ++p) {
                ref += hA[static_cast<std::size_t>(i) * k + p] * hB[static_cast<std::size_t>(p) * n + j];
            }
            wrong += (hC[static_cast<std::size_t>(i) * n + j] != ref) ? 1 : 0;   // small integers: exact
        }
    }
    std::printf("cublasSgemm %d x %d x %d: %d elements differ from the CPU loops\n", m, n, k, wrong);
    checkBlas(cublasDestroy(handle), "cublasDestroy");
    check(cudaFree(dA), "cudaFree A");
    check(cudaFree(dB), "cudaFree B");
    check(cudaFree(dC), "cudaFree C");
    return 0;
}
