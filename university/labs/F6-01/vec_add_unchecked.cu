// F6-01 forensic evidence: the same program as Listing 1 with every check removed
// (a deliberately broken version; do not copy it). It prints the first results.
#include <cstdio>
#include <vector>
#include <cuda_runtime.h>

__global__ void vecAdd(const float* A, const float* B, float* C, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        C[i] = A[i] + B[i];
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
    cudaMalloc(&dA, bytes);
    cudaMalloc(&dB, bytes);
    cudaMalloc(&dC, bytes);
    cudaMemcpy(dA, hA.data(), bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(dB, hB.data(), bytes, cudaMemcpyHostToDevice);
    vecAdd<<<(n + 255) / 256, 256>>>(dA, dB, dC, n);
    cudaMemcpy(hC.data(), dC, bytes, cudaMemcpyDeviceToHost);

    for (int i = 0; i < 5; ++i) {
        std::printf("C[%d] = %.1f (expected %.1f)\n", i, hC[i], hA[i] + hB[i]);
    }
    std::printf("program finished normally\n");
    cudaFree(dA);
    cudaFree(dB);
    cudaFree(dC);
    return 0;
}
