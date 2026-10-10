// F6-01 Listing 1: vector add, in the style of the CUDA Programming Guide's vecAdd
// example (same index formula and bounds check), extended with allocation, copies,
// error checking and a correctness check against the CPU.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

__global__ void vecAdd(const float* A, const float* B, float* C, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        C[i] = A[i] + B[i];
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1 << 20;
    const std::size_t bytes = n * sizeof(float);
    std::vector<float> hA(n), hB(n), hC(n);
    for (int i = 0; i < n; ++i) { hA[i] = 1.0f * i; hB[i] = 2.0f * i; }

    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    check(cudaMalloc(&dA, bytes), "cudaMalloc A");
    check(cudaMalloc(&dB, bytes), "cudaMalloc B");
    check(cudaMalloc(&dC, bytes), "cudaMalloc C");
    check(cudaMemcpy(dA, hA.data(), bytes, cudaMemcpyHostToDevice), "copy A to device");
    check(cudaMemcpy(dB, hB.data(), bytes, cudaMemcpyHostToDevice), "copy B to device");

    const int threadsPerBlock = 256;
    const int blocks = (n + threadsPerBlock - 1) / threadsPerBlock;
    vecAdd<<<blocks, threadsPerBlock>>>(dA, dB, dC, n);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hC.data(), dC, bytes, cudaMemcpyDeviceToHost), "copy C to host");

    int errors = 0;
    for (int i = 0; i < n; ++i) {
        if (hC[i] != hA[i] + hB[i]) { ++errors; }
    }
    std::printf("checked %d elements, %d errors\n", n, errors);
    check(cudaFree(dA), "cudaFree A");
    check(cudaFree(dB), "cudaFree B");
    check(cudaFree(dC), "cudaFree C");
    return errors == 0 ? 0 : 1;
}
