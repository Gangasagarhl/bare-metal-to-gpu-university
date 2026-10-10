// F6-14 forensic evidence: the byte-sum kernel from the scenario (sumBytes32) and the fix
// (sumBytes64). The only difference is the type of the index i. Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

constexpr int BLOCK = 256;

__device__ unsigned blockSumU(unsigned v)
{
    __shared__ unsigned s[BLOCK];
    s[threadIdx.x] = v;
    __syncthreads();
    for (unsigned stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (threadIdx.x < stride) { s[threadIdx.x] += s[threadIdx.x + stride]; }
        __syncthreads();
    }
    return s[0];
}

__global__ void sumBytes32(const unsigned char* in, size_t n, unsigned long long* total)
{
    int i = blockIdx.x * (blockDim.x * 2) + threadIdx.x;          // 32-bit index
    unsigned v = 0;
    if (i < n) { v += in[i]; }
    if (i + blockDim.x < n) { v += in[i + blockDim.x]; }
    unsigned s = blockSumU(v);
    if (threadIdx.x == 0) { atomicAdd(total, static_cast<unsigned long long>(s)); }
}

__global__ void sumBytes64(const unsigned char* in, size_t n, unsigned long long* total)
{
    size_t i = static_cast<size_t>(blockIdx.x) * (blockDim.x * 2) + threadIdx.x;   // 64-bit index
    unsigned v = 0;
    if (i < n) { v += in[i]; }
    if (i + blockDim.x < n) { v += in[i + blockDim.x]; }
    unsigned s = blockSumU(v);
    if (threadIdx.x == 0) { atomicAdd(total, static_cast<unsigned long long>(s)); }
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
    const size_t n = 3ull << 30;                                   // 3 GiB of bytes
    unsigned char* dIn = nullptr;
    unsigned long long* dTotal = nullptr;
    check(cudaMalloc(&dIn, n), "cudaMalloc in");
    check(cudaMalloc(&dTotal, sizeof(unsigned long long)), "cudaMalloc total");
    check(cudaMemset(dIn, 1, n), "memset in");                      // every byte = 1, so the sum is n
    const unsigned blocks = static_cast<unsigned>((n + 2 * BLOCK - 1) / (2 * BLOCK));
    for (int v = 0; v < 2; ++v) {
        check(cudaMemset(dTotal, 0, sizeof(unsigned long long)), "memset total");
        if (v == 0) { sumBytes32<<<blocks, BLOCK>>>(dIn, n, dTotal); }
        else { sumBytes64<<<blocks, BLOCK>>>(dIn, n, dTotal); }
        check(cudaGetLastError(), "launch");
        unsigned long long got = 0;
        check(cudaMemcpy(&got, dTotal, sizeof got, cudaMemcpyDeviceToHost), "copy total");
        std::printf("%s: %llu (expected %zu)\n", v == 0 ? "sumBytes32" : "sumBytes64", got, n);
    }
    check(cudaFree(dIn), "free in");
    check(cudaFree(dTotal), "free total");
    return 0;
}
