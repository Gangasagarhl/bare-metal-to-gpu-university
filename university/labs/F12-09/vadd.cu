// vadd.cu - CI stage "gpu": a vector add whose build is checked on every commit.
//   vadd          run the kernel on 1,000,000 elements and check the result on the CPU
//   vadd --probe  only ask the runtime how many GPUs there are (exit 77 = none: skip)
#include <cstdio>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>

static bool check(cudaError_t e, const char* what)
{
    if (e != cudaSuccess) {
        std::printf("%s failed: %s\n", what, cudaGetErrorString(e));
        return false;
    }
    return true;
}

__global__ void vecAdd(const float* a, const float* b, float* c, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main(int argc, char** argv)
{
    int count = 0;
    if (!check(cudaGetDeviceCount(&count), "cudaGetDeviceCount") || count == 0) {
        std::printf("gpu: no usable CUDA device on this machine\n");
        return (argc > 1 && std::strcmp(argv[1], "--probe") == 0) ? 77 : 1;
    }
    if (argc > 1) {
        std::printf("gpu: %d device(s)\n", count);
        return 0;
    }
    const int n = 1000000;
    std::vector<float> hA(n, 1.0f), hB(n, 2.0f), hC(n, 0.0f);
    float *dA = nullptr, *dB = nullptr, *dC = nullptr;
    const size_t bytes = n * sizeof(float);
    bool ok = check(cudaMalloc(&dA, bytes), "cudaMalloc") &&
              check(cudaMalloc(&dB, bytes), "cudaMalloc") &&
              check(cudaMalloc(&dC, bytes), "cudaMalloc") &&
              check(cudaMemcpy(dA, hA.data(), bytes, cudaMemcpyHostToDevice), "cudaMemcpy") &&
              check(cudaMemcpy(dB, hB.data(), bytes, cudaMemcpyHostToDevice), "cudaMemcpy");
    if (ok) {
        vecAdd<<<(n + 255) / 256, 256>>>(dA, dB, dC, n);
        ok = check(cudaGetLastError(), "launch") && check(cudaDeviceSynchronize(), "kernel") &&
             check(cudaMemcpy(hC.data(), dC, bytes, cudaMemcpyDeviceToHost), "cudaMemcpy");
    }
    int wrong = 0;
    for (int i = 0; ok && i < n; ++i) {
        wrong += hC[i] == 3.0f ? 0 : 1;
    }
    cudaFree(dA);
    cudaFree(dB);
    cudaFree(dC);
    std::printf("vecAdd: %s, %d wrong elements\n", ok ? "ran" : "did not run", wrong);
    return (ok && wrong == 0) ? 0 : 1;
}
