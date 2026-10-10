// F6-17 Listing 1: a 256-bin histogram of bytes, three ways, checked against a CPU count and
// against cub::DeviceHistogram::HistogramEven (CUB 2.0.1).
//   histGlobal  - one global atomicAdd per byte
//   histShared  - privatized: each block counts in shared memory, then merges its 256 bins
//   histStride  - privatized + a fixed grid that walks the input (fewer blocks, fewer merges)
// Built for real; untested on hardware.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cub/cub.cuh>
#include <cuda_runtime.h>

constexpr int BINS = 256;
constexpr int BLOCK = 256;

__global__ void histGlobal(const unsigned char* in, size_t n, unsigned* bins)
{
    size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i < n) { atomicAdd(&bins[in[i]], 1u); }
}

__global__ void histShared(const unsigned char* in, size_t n, unsigned* bins)
{
    __shared__ unsigned local[BINS];
    for (int b = threadIdx.x; b < BINS; b += blockDim.x) { local[b] = 0; }     // 1. zero
    __syncthreads();
    size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i < n) { atomicAdd(&local[in[i]], 1u); }                               // 2. count
    __syncthreads();
    for (int b = threadIdx.x; b < BINS; b += blockDim.x) {                     // 3. merge
        if (local[b] != 0) { atomicAdd(&bins[b], local[b]); }
    }
}

__global__ void histStride(const unsigned char* in, size_t n, unsigned* bins)
{
    __shared__ unsigned local[BINS];
    for (int b = threadIdx.x; b < BINS; b += blockDim.x) { local[b] = 0; }
    __syncthreads();
    for (size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        atomicAdd(&local[in[i]], 1u);
    }
    __syncthreads();
    for (int b = threadIdx.x; b < BINS; b += blockDim.x) {
        if (local[b] != 0) { atomicAdd(&bins[b], local[b]); }
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
    int dev = 0, sms = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    check(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, dev), "SM count");
    const size_t n = size_t{1} << 26;
    const char* inputs[] = {"uniform", "skewed (90 % zeros)"};
    for (int kind = 0; kind < 2; ++kind) {
        std::vector<unsigned char> h(n);
        std::vector<unsigned> ref(BINS, 0);
        for (size_t i = 0; i < n; ++i) {
            unsigned r = static_cast<unsigned>(i * 2654435761u);
            h[i] = (kind == 1 && r % 10 != 0) ? 0 : static_cast<unsigned char>(r >> 24);
            ++ref[h[i]];
        }
        unsigned char* dIn = nullptr;
        unsigned* dBins = nullptr;
        check(cudaMalloc(&dIn, n), "malloc in");
        check(cudaMalloc(&dBins, sizeof(unsigned) * BINS), "malloc bins");
        check(cudaMemcpy(dIn, h.data(), n, cudaMemcpyHostToDevice), "copy in");
        const unsigned blocks = static_cast<unsigned>((n + BLOCK - 1) / BLOCK);
        const char* names[] = {"histGlobal", "histShared", "histStride", "CUB HistogramEven"};
        for (int v = 0; v < 4; ++v) {
            check(cudaMemset(dBins, 0, sizeof(unsigned) * BINS), "memset bins");
            cudaEvent_t start, stop;
            check(cudaEventCreate(&start), "event");
            check(cudaEventCreate(&stop), "event");
            void* dTemp = nullptr;
            size_t tempBytes = 0;
            if (v == 3) {
                check(cub::DeviceHistogram::HistogramEven(dTemp, tempBytes, dIn, dBins, BINS + 1, 0, BINS,
                                                          static_cast<int>(n)), "CUB size query");
                check(cudaMalloc(&dTemp, tempBytes), "malloc temp");
            }
            check(cudaEventRecord(start), "record");
            if (v == 0) { histGlobal<<<blocks, BLOCK>>>(dIn, n, dBins); }
            if (v == 1) { histShared<<<blocks, BLOCK>>>(dIn, n, dBins); }
            if (v == 2) { histStride<<<sms * 4, BLOCK>>>(dIn, n, dBins); }
            if (v == 3) {
                check(cub::DeviceHistogram::HistogramEven(dTemp, tempBytes, dIn, dBins, BINS + 1, 0, BINS,
                                                          static_cast<int>(n)), "CUB HistogramEven");
            }
            check(cudaGetLastError(), names[v]);
            check(cudaEventRecord(stop), "record");
            check(cudaEventSynchronize(stop), "sync");
            float ms = 0.0f;
            check(cudaEventElapsedTime(&ms, start, stop), "elapsed");
            std::vector<unsigned> got(BINS);
            check(cudaMemcpy(got.data(), dBins, sizeof(unsigned) * BINS, cudaMemcpyDeviceToHost), "copy bins");
            std::printf("%-20s %-18s %s, %.3f ms (one run)\n", inputs[kind], names[v],
                        got == ref ? "matches the CPU count" : "WRONG", ms);
            if (dTemp != nullptr) { check(cudaFree(dTemp), "free temp"); }
            check(cudaEventDestroy(start), "event");
            check(cudaEventDestroy(stop), "event");
        }
        check(cudaFree(dIn), "free in");
        check(cudaFree(dBins), "free bins");
    }
    return 0;
}
