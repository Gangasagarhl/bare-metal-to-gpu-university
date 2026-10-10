// BR-03 Listing 2: the same 256-bin byte histogram in CUDA, five kernels.
//   histRacy      ++bins[b] in global memory without atomics: the CPU data race, on the GPU
//   histOneThread one GPU thread walks the whole input (launched <<<1, 1>>>)
//   histChunked   CPU habit: each thread counts its own contiguous slice of kChunk bytes
//   histGlobal    one thread per byte, neighbouring threads read neighbouring bytes, global atomics
//   histShared    per-block bins in shared memory, a grid-stride loop, one merge per block
// Built for real in this build; untested on hardware (no GPU).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int kBins = 256;
constexpr int kBlock = 256;
constexpr int kChunk = 64;

__global__ void histRacy(const unsigned char* in, size_t n, unsigned* bins)
{
    size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i < n) { ++bins[in[i]]; }                               // data race: lost updates
}

__global__ void histOneThread(const unsigned char* in, size_t n, unsigned* bins)
{
    for (size_t i = 0; i < n; ++i) { ++bins[in[i]]; }           // correct, and alone
}

__global__ void histChunked(const unsigned char* in, size_t n, unsigned* bins)
{
    size_t t = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    size_t begin = t * kChunk;                                  // thread t: bytes [64t, 64t + 64)
    for (size_t i = begin; i < begin + kChunk && i < n; ++i) { atomicAdd(&bins[in[i]], 1u); }
}

__global__ void histGlobal(const unsigned char* in, size_t n, unsigned* bins)
{
    size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i < n) { atomicAdd(&bins[in[i]], 1u); }                 // neighbours read neighbours
}

__global__ void histShared(const unsigned char* in, size_t n, unsigned* bins)
{
    __shared__ unsigned local[kBins];                           // the row group's own table
    for (int b = threadIdx.x; b < kBins; b += blockDim.x) { local[b] = 0; }
    __syncthreads();                                            // cheap: one block, one SM
    for (size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        atomicAdd(&local[in[i]], 1u);                           // shared-memory atomic
    }
    __syncthreads();
    for (int b = threadIdx.x; b < kBins; b += blockDim.x) {     // merge: one global add per bin
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
    const size_t n = size_t{1} << 24;
    std::vector<unsigned char> h(n);
    std::vector<unsigned> ref(kBins, 0);
    for (size_t i = 0; i < n; ++i) {
        h[i] = static_cast<unsigned char>((static_cast<unsigned>(i) * 2654435761u) >> 24);
        ++ref[h[i]];
    }
    std::printf("CPU reference ready: %zu bytes, bin 0 = %u\n", n, ref[0]);

    cudaDeviceProp prop{};
    check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");
    std::printf("GPU: %s, %d SMs, warpSize %d\n", prop.name, prop.multiProcessorCount, prop.warpSize);
    unsigned char* dIn = nullptr;
    unsigned* dBins = nullptr;
    check(cudaMalloc(&dIn, n), "cudaMalloc in");
    check(cudaMalloc(&dBins, sizeof(unsigned) * kBins), "cudaMalloc bins");
    check(cudaMemcpy(dIn, h.data(), n, cudaMemcpyHostToDevice), "copy in");
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "event");
    check(cudaEventCreate(&stop), "event");

    const unsigned perByte = static_cast<unsigned>((n + kBlock - 1) / kBlock);
    const unsigned perChunk = static_cast<unsigned>((n / kChunk + kBlock - 1) / kBlock);
    const unsigned fixedGrid = static_cast<unsigned>(prop.multiProcessorCount) * 4;
    const char* names[] = {"histRacy", "histOneThread", "histChunked", "histGlobal", "histShared"};
    for (int k = 0; k < 5; ++k) {
        const int runs = (k == 1) ? 1 : 6;                      // one warm-up + five timed runs
        std::vector<float> ms;
        for (int r = 0; r < runs; ++r) {
            check(cudaMemset(dBins, 0, sizeof(unsigned) * kBins), "memset");
            check(cudaEventRecord(start), "record");
            if (k == 0) { histRacy<<<perByte, kBlock>>>(dIn, n, dBins); }
            if (k == 1) { histOneThread<<<1, 1>>>(dIn, n, dBins); }
            if (k == 2) { histChunked<<<perChunk, kBlock>>>(dIn, n, dBins); }
            if (k == 3) { histGlobal<<<perByte, kBlock>>>(dIn, n, dBins); }
            if (k == 4) { histShared<<<fixedGrid, kBlock>>>(dIn, n, dBins); }
            check(cudaGetLastError(), names[k]);
            check(cudaEventRecord(stop), "record");
            check(cudaEventSynchronize(stop), "sync");
            float t = 0.0f;
            check(cudaEventElapsedTime(&t, start, stop), "elapsed");
            if (runs == 1 || r > 0) { ms.push_back(t); }
        }
        std::sort(ms.begin(), ms.end());
        std::vector<unsigned> got(kBins);
        check(cudaMemcpy(got.data(), dBins, sizeof(unsigned) * kBins, cudaMemcpyDeviceToHost), "copy bins");
        std::printf("%-14s %s, median %.3f ms of %zu run(s)\n", names[k],
                    got == ref ? "matches the CPU count" : "WRONG", ms[ms.size() / 2], ms.size());
    }
    check(cudaFree(dIn), "free");
    check(cudaFree(dBins), "free");
    return 0;
}
