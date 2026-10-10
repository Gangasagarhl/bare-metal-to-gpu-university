// F6-18 Listing 3: a 1-bit split radix sort in CUDA (32 passes for 32-bit keys), built on a
// block scan of the 0-flags and a device-wide scan of the per-block 0-counts, checked against
// cub::DeviceRadixSort::SortKeys (CUB 2.0.1). Simple, stable, not fast. Untested on hardware.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <cub/cub.cuh>
#include <cuda_runtime.h>

constexpr int BLOCK = 256;

__device__ int blockExclusiveScan(int flag, int* total)
{
    __shared__ int s[BLOCK];
    s[threadIdx.x] = flag;
    __syncthreads();
    for (int d = 1; d < BLOCK; d *= 2) {                    // Hillis-Steele with a copy (F6-16)
        int add = (threadIdx.x >= static_cast<unsigned>(d)) ? s[threadIdx.x - d] : 0;
        __syncthreads();
        s[threadIdx.x] += add;
        __syncthreads();
    }
    *total = s[BLOCK - 1];
    return s[threadIdx.x] - flag;                           // inclusive minus own = exclusive
}

__global__ void countZeros(const unsigned* keys, int n, int bit, int* zerosPerBlock)
{
    int i = blockIdx.x * BLOCK + threadIdx.x;
    int isZero = (i < n) && (((keys[i] >> bit) & 1u) == 0);
    int z = __syncthreads_count(isZero);                    // block-wide count in one barrier
    if (threadIdx.x == 0) { zerosPerBlock[blockIdx.x] = z; }
}

__global__ void scatter(const unsigned* in, unsigned* out, int n, int bit,
                        const int* zerosBefore, const int* zerosPerBlock, int blocks)
{
    int i = blockIdx.x * BLOCK + threadIdx.x;
    unsigned key = (i < n) ? in[i] : 0u;
    int isZero = (i < n) && (((key >> bit) & 1u) == 0);
    int tileZeros = 0;
    int zRank = blockExclusiveScan(isZero, &tileZeros);   // 0-keys before me in my tile
    int totalZeros = zerosBefore[blocks - 1] + zerosPerBlock[blocks - 1];
    if (i < n) {
        int tileStart = blockIdx.x * BLOCK;
        int onesBeforeTile = tileStart - zerosBefore[blockIdx.x];
        int oRank = threadIdx.x - zRank;                    // 1-keys before me in my tile
        int dst = isZero ? zerosBefore[blockIdx.x] + zRank : totalZeros + onesBeforeTile + oRank;
        out[dst] = key;
    }
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
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    const int n = 1000003, blocks = (n + BLOCK - 1) / BLOCK;
    std::mt19937 rng(5);
    std::vector<unsigned> h(n);
    for (auto& k : h) { k = static_cast<unsigned>(rng()); }
    std::vector<unsigned> ref = h;
    std::sort(ref.begin(), ref.end());
    unsigned *dA, *dB, *dCub;
    int *dZeros, *dBefore;
    check(cudaMalloc(&dA, sizeof(unsigned) * n), "malloc");
    check(cudaMalloc(&dB, sizeof(unsigned) * n), "malloc");
    check(cudaMalloc(&dCub, sizeof(unsigned) * n), "malloc");
    check(cudaMalloc(&dZeros, sizeof(int) * blocks), "malloc");
    check(cudaMalloc(&dBefore, sizeof(int) * blocks), "malloc");
    check(cudaMemcpy(dA, h.data(), sizeof(unsigned) * n, cudaMemcpyHostToDevice), "copy");
    void* dTemp = nullptr;
    size_t tempBytes = 0;
    check(cub::DeviceScan::ExclusiveSum(dTemp, tempBytes, dZeros, dBefore, blocks), "scan size query");
    check(cudaMalloc(&dTemp, tempBytes), "malloc temp");
    for (int bit = 0; bit < 32; ++bit) {
        countZeros<<<blocks, BLOCK>>>(dA, n, bit, dZeros);
        check(cudaGetLastError(), "countZeros");
        check(cub::DeviceScan::ExclusiveSum(dTemp, tempBytes, dZeros, dBefore, blocks), "scan");
        scatter<<<blocks, BLOCK>>>(dA, dB, n, bit, dBefore, dZeros, blocks);
        check(cudaGetLastError(), "scatter");
        std::swap(dA, dB);
    }
    std::vector<unsigned> mine(n), viaCub(n);
    check(cudaMemcpy(mine.data(), dA, sizeof(unsigned) * n, cudaMemcpyDeviceToHost), "copy");
    void* dTemp2 = nullptr;
    size_t temp2 = 0;
    check(cudaMemcpy(dB, h.data(), sizeof(unsigned) * n, cudaMemcpyHostToDevice), "copy");
    check(cub::DeviceRadixSort::SortKeys(dTemp2, temp2, dB, dCub, n), "SortKeys size query");
    check(cudaMalloc(&dTemp2, temp2), "malloc temp2");
    check(cub::DeviceRadixSort::SortKeys(dTemp2, temp2, dB, dCub, n), "SortKeys");
    check(cudaMemcpy(viaCub.data(), dCub, sizeof(unsigned) * n, cudaMemcpyDeviceToHost), "copy");
    std::printf("split radix sort: %s; CUB SortKeys: %s\n", mine == ref ? "pass" : "FAIL", viaCub == ref ? "pass" : "FAIL");
    for (void* p : {static_cast<void*>(dA), static_cast<void*>(dB), static_cast<void*>(dCub),
                    static_cast<void*>(dZeros), static_cast<void*>(dBefore), dTemp, dTemp2}) {
        check(cudaFree(p), "free");
    }
    return 0;
}
