// F6-34 Listing 1: small kernels chosen so that each shows one thing in PTX and SASS.
// The program runs each once and checks the results (needs a GPU); the lab reads
// their PTX and SASS, which needs no GPU.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"

// 1. Plain pointers: the compiler must assume x and y may overlap.
__global__ void axpyPlain(int n, float a, const float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

// 2. The same with __restrict__: x is read-only and does not overlap y.
__global__ void axpyRestrict(int n, float a, const float* __restrict__ x, float* __restrict__ y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

// 3. Four floats per thread through float4: one wide load and one wide store.
__global__ void copy4(const float4* __restrict__ in, float4* __restrict__ out, int n4)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n4) {
        out[i] = in[i];
    }
}

// 4. A block sum in shared memory: shared loads/stores and barriers.
__global__ void blockSum(const float* __restrict__ in, float* __restrict__ out, int n)
{
    __shared__ float tile[256];
    int t = threadIdx.x;
    int i = blockIdx.x * blockDim.x + t;
    tile[t] = (i < n) ? in[i] : 0.0f;
    __syncthreads();
    for (int stride = 128; stride > 0; stride /= 2) {
        if (t < stride) {
            tile[t] += tile[t + stride];
        }
        __syncthreads();
    }
    if (t == 0) {
        out[blockIdx.x] = tile[0];
    }
}

int main()
{
    const int n = 1 << 20;
    const int threads = 256;
    const int blocks = n / threads;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    std::vector<float> h(static_cast<std::size_t>(n), 1.0f);
    float* dX = nullptr;
    float* dY = nullptr;
    float* dS = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));
    CUDA_CHECK(cudaMalloc(&dS, static_cast<std::size_t>(blocks) * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(dX, h.data(), bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dY, h.data(), bytes, cudaMemcpyHostToDevice));
    axpyPlain<<<blocks, threads>>>(n, 2.0f, dX, dY);       // y = 3
    axpyRestrict<<<blocks, threads>>>(n, 2.0f, dX, dY);    // y = 5
    copy4<<<blocks / 4, threads>>>(reinterpret_cast<const float4*>(dY),
                                   reinterpret_cast<float4*>(dX), n / 4);
    blockSum<<<blocks, threads>>>(dX, dS, n);              // each block: 256 * 5
    CUDA_CHECK_LAUNCH();
    std::vector<float> sums(static_cast<std::size_t>(blocks));
    CUDA_CHECK(cudaMemcpy(sums.data(), dS, sums.size() * sizeof(float), cudaMemcpyDeviceToHost));
    int errors = 0;
    for (float s : sums) {
        if (s != 1280.0f) {
            ++errors;
        }
    }
    std::printf("%d blocks, %d wrong block sums\n", blocks, errors);
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    CUDA_CHECK(cudaFree(dS));
    return errors == 0 ? 0 : 1;
}
