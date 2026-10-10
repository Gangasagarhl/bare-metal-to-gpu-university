// BR-02 Listing 5: a CUDA program that calls a LIBRARY (cuBLAS), used to test what a
// text translator does with library calls. Built with nvcc -lcublas in run.sh.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>
#include <cublas_v2.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, cudaGetErrorName(err));
        std::exit(EXIT_FAILURE);
    }
}

static void checkBlas(cublasStatus_t st, const char* what)
{
    if (st != CUBLAS_STATUS_SUCCESS) {
        std::fprintf(stderr, "%s failed: cuBLAS status %d\n", what, static_cast<int>(st));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 2;                                     // C = A * B, 2 x 2, column-major
    const std::vector<float> hA{1, 2, 3, 4}, hB{5, 6, 7, 8};
    std::vector<float> hC(n * n, 0.0f);
    const std::size_t bytes = n * n * sizeof(float);
    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    check(cudaMalloc(&dA, bytes), "cudaMalloc A");
    check(cudaMalloc(&dB, bytes), "cudaMalloc B");
    check(cudaMalloc(&dC, bytes), "cudaMalloc C");
    check(cudaMemcpy(dA, hA.data(), bytes, cudaMemcpyHostToDevice), "copy A");
    check(cudaMemcpy(dB, hB.data(), bytes, cudaMemcpyHostToDevice), "copy B");
    cublasHandle_t handle = nullptr;
    checkBlas(cublasCreate(&handle), "cublasCreate");
    const float alpha = 1.0f;
    const float beta = 0.0f;
    checkBlas(cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, n, n, n, &alpha, dA, n, dB, n,
                          &beta, dC, n), "cublasSgemm");
    checkBlas(cublasDestroy(handle), "cublasDestroy");
    check(cudaMemcpy(hC.data(), dC, bytes, cudaMemcpyDeviceToHost), "copy C");
    std::printf("C = [%g %g; %g %g]\n", hC[0], hC[2], hC[1], hC[3]);
    check(cudaFree(dA), "cudaFree A");
    check(cudaFree(dB), "cudaFree B");
    check(cudaFree(dC), "cudaFree C");
    return 0;
}
