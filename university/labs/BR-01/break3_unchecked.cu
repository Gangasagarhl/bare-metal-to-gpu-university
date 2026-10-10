// BR-01 Listing 9 (break 3): the port with every check removed ("it is shorter now").
// It compiles and finishes normally. Read its output before you trust it.
#include <cstddef>
#include <cstdio>
#include <vector>
#include <cuda_runtime.h>

template <typename T>
__host__ __device__ T add(T a, T b)
{
    return a + b;
}

template <typename T>
__global__ void vecAdd(const T* a, const T* b, T* c, std::size_t n)
{
    const std::size_t i = blockIdx.x * static_cast<std::size_t>(blockDim.x) + threadIdx.x;
    if (i < n) {
        c[i] = add(a[i], b[i]);
    }
}

int main()
{
    const std::size_t n = 1000003;
    const std::size_t bytes = n * sizeof(float);
    std::vector<float> hA(n), hB(n), hC(n);
    for (std::size_t i = 0; i < n; ++i) {
        hA[i] = 1.0f * static_cast<float>(i);
        hB[i] = 2.0f * static_cast<float>(i);
    }
    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    cudaMalloc(&dA, bytes);                         // return values ignored
    cudaMalloc(&dB, bytes);
    cudaMalloc(&dC, bytes);
    cudaMemcpy(dA, hA.data(), bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(dB, hB.data(), bytes, cudaMemcpyHostToDevice);
    const unsigned threadsPerBlock = 256;
    const unsigned blocks = static_cast<unsigned>((n + threadsPerBlock - 1) / threadsPerBlock);
    vecAdd<<<blocks, threadsPerBlock>>>(dA, dB, dC, n);
    cudaMemcpy(hC.data(), dC, bytes, cudaMemcpyDeviceToHost);
    std::printf("c[0] = %.1f, c[1] = %.1f, c[2] = %.1f, c[n-1] = %.1f\n",
                hC[0], hC[1], hC[2], hC[n - 1]);
    std::printf("finished normally\n");
    cudaFree(dA);
    cudaFree(dB);
    cudaFree(dC);
    return 0;
}
