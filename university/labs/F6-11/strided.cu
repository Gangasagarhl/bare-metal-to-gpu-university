// F6-11 Listing 2: a kernel whose only job is to read shared memory with a chosen stride,
// so that a profiler (or the event timer below) can see bank conflicts on a real GPU.
// Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

constexpr int WORDS = 32 * 33;            // shared array size in 4-byte words (chosen by us)
constexpr int REPEAT = 4096;              // reads per thread, so the reads dominate the time

__global__ void readStrided(int* out, int stride)
{
    __shared__ int a[WORDS];
    for (int k = threadIdx.x; k < WORDS; k += blockDim.x) {
        a[k] = k;
    }
    __syncthreads();
    int lane = threadIdx.x % 32;
    int idx = (lane * stride) % WORDS;
    int sum = 0;
    for (int r = 0; r < REPEAT; ++r) {
        sum += a[idx];                    // the access under test
        idx = (idx + 1) % WORDS;          // move on so the compiler cannot hoist the read
    }
    out[blockIdx.x * blockDim.x + threadIdx.x] = sum;
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
    const int blocks = 1024, threads = 256;
    int* dOut = nullptr;
    check(cudaMalloc(&dOut, sizeof(int) * blocks * threads), "cudaMalloc out");
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "event create");
    check(cudaEventCreate(&stop), "event create");
    std::printf("stride  time_ms (median of 20 runs is the course protocol; one run shown here)\n");
    for (int stride : {1, 2, 4, 8, 16, 32, 33}) {
        readStrided<<<blocks, threads>>>(dOut, stride);          // warm-up
        check(cudaGetLastError(), "warm-up launch");
        check(cudaEventRecord(start), "record start");
        readStrided<<<blocks, threads>>>(dOut, stride);
        check(cudaGetLastError(), "timed launch");
        check(cudaEventRecord(stop), "record stop");
        check(cudaEventSynchronize(stop), "synchronize");
        float ms = 0.0f;
        check(cudaEventElapsedTime(&ms, start, stop), "elapsed");
        std::printf("%-7d %.3f\n", stride, ms);
    }
    check(cudaEventDestroy(start), "event destroy");
    check(cudaEventDestroy(stop), "event destroy");
    check(cudaFree(dOut), "cudaFree out");
    return 0;
}
