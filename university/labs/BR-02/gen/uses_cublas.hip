// BR-02 Listing 5: a CUDA program that calls a LIBRARY (cuBLAS), used to test what a
// text translator does with library calls. Built with nvcc -lcublas in run.sh.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>
#include <cublas_v2.h>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, hipGetErrorName(err));
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
    check(hipMalloc(&dA, bytes), "hipMalloc A");
    check(hipMalloc(&dB, bytes), "hipMalloc B");
    check(hipMalloc(&dC, bytes), "hipMalloc C");
    check(hipMemcpy(dA, hA.data(), bytes, hipMemcpyHostToDevice), "copy A");
    check(hipMemcpy(dB, hB.data(), bytes, hipMemcpyHostToDevice), "copy B");
    cublasHandle_t handle = nullptr;
    checkBlas(cublasCreate(&handle), "cublasCreate");
    const float alpha = 1.0f;
    const float beta = 0.0f;
    checkBlas(cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, n, n, n, &alpha, dA, n, dB, n,
                          &beta, dC, n), "cublasSgemm");
    checkBlas(cublasDestroy(handle), "cublasDestroy");
    check(hipMemcpy(hC.data(), dC, bytes, hipMemcpyDeviceToHost), "copy C");
    std::printf("C = [%g %g; %g %g]\n", hC[0], hC[2], hC[1], hC[3]);
    check(hipFree(dA), "hipFree A");
    check(hipFree(dB), "hipFree B");
    check(hipFree(dC), "hipFree C");
    return 0;
}
