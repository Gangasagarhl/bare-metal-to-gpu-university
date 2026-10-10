// F6-20 Listing 2: the same work (y = a * x) written with 1, 2, 4 or 8 independent
// elements per thread (instruction-level parallelism, ILP). The host program asks the
// runtime for each version's occupancy and times it. Untested on hardware: the build
// container has no GPU, so the run stops at the first runtime call with its error.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// Each thread handles ILP elements that are blockDim.x apart, so a warp's loads stay
// coalesced. All ILP loads are issued before the first multiply needs one of them.
template <int ILP>
__global__ void scaleIlp(const float* __restrict__ x, float* __restrict__ y, float a, int n)
{
    const int base = blockIdx.x * blockDim.x * ILP + threadIdx.x;
    float v[ILP];
#pragma unroll
    for (int i = 0; i < ILP; ++i) {
        const int idx = base + i * blockDim.x;
        v[i] = idx < n ? x[idx] : 0.0f;
    }
#pragma unroll
    for (int i = 0; i < ILP; ++i) {
        const int idx = base + i * blockDim.x;
        if (idx < n) {
            y[idx] = a * v[i];
        }
    }
}

template <int ILP>
void measure(const float* dX, float* dY, int n, int threads)
{
    int blocksPerSM = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocksPerSM, scaleIlp<ILP>, threads, 0),
          "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
    cudaDeviceProp prop;
    check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");
    const double occupancy = 100.0 * blocksPerSM * threads / prop.maxThreadsPerMultiProcessor;
    const int blocks = (n + threads * ILP - 1) / (threads * ILP);
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "cudaEventCreate");
    check(cudaEventCreate(&stop), "cudaEventCreate");
    std::vector<float> ms;
    for (int rep = 0; rep < 21; ++rep) {          // first run is a warm-up, then 20 timed
        check(cudaEventRecord(start), "cudaEventRecord");
        scaleIlp<ILP><<<blocks, threads>>>(dX, dY, 2.0f, n);
        check(cudaGetLastError(), "launch scaleIlp");
        check(cudaEventRecord(stop), "cudaEventRecord");
        check(cudaEventSynchronize(stop), "cudaEventSynchronize");
        float t = 0.0f;
        check(cudaEventElapsedTime(&t, start, stop), "cudaEventElapsedTime");
        if (rep > 0) {
            ms.push_back(t);
        }
    }
    std::sort(ms.begin(), ms.end());
    const double median = ms[ms.size() / 2];
    const double gbps = 2.0 * n * sizeof(float) / (median * 1e-3) / 1e9;
    std::printf("ILP %d  threads %4d  occupancy %5.1f %%  median %8.4f ms  %7.1f GB/s\n", ILP,
                threads, occupancy, median, gbps);
    check(cudaEventDestroy(start), "cudaEventDestroy");
    check(cudaEventDestroy(stop), "cudaEventDestroy");
}

int main()
{
    const int n = 1 << 26;
    float* dX = nullptr;
    float* dY = nullptr;
    check(cudaMalloc(&dX, n * sizeof(float)), "cudaMalloc dX");
    check(cudaMalloc(&dY, n * sizeof(float)), "cudaMalloc dY");
    check(cudaMemset(dX, 0, n * sizeof(float)), "cudaMemset");
    for (int threads : {64, 128, 256, 512}) {
        measure<1>(dX, dY, n, threads);
        measure<2>(dX, dY, n, threads);
        measure<4>(dX, dY, n, threads);
        measure<8>(dX, dY, n, threads);
    }
    check(cudaFree(dX), "cudaFree");
    check(cudaFree(dY), "cudaFree");
    return 0;
}
