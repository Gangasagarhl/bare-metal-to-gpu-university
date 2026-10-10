// F6-14 Listing 1: six reduction kernels, from naive to warp shuffles (curriculum milestone E3).
// Each step changes one thing; the host checks every version and times it (events, median of 20).
// Built for real; untested on hardware (no GPU in the build container).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int BLOCK = 256;   // threads per block (a power of two, chosen by us)

// v1: interleaved addressing; the modulo test makes lanes of one warp take different paths
template <typename T>
__global__ void reduceV1(const T* in, T* out, int n)
{
    __shared__ T s[BLOCK];
    unsigned tid = threadIdx.x;
    unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
    s[tid] = (i < static_cast<unsigned>(n)) ? in[i] : T(0);
    __syncthreads();
    for (unsigned stride = 1; stride < blockDim.x; stride *= 2) {
        if (tid % (2 * stride) == 0) { s[tid] += s[tid + stride]; }
        __syncthreads();
    }
    if (tid == 0) { out[blockIdx.x] = s[0]; }
}

// v2: same pairs, but consecutive threads do the work (no divergence; strided shared accesses)
template <typename T>
__global__ void reduceV2(const T* in, T* out, int n)
{
    __shared__ T s[BLOCK];
    unsigned tid = threadIdx.x;
    unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
    s[tid] = (i < static_cast<unsigned>(n)) ? in[i] : T(0);
    __syncthreads();
    for (unsigned stride = 1; stride < blockDim.x; stride *= 2) {
        unsigned index = 2 * stride * tid;
        if (index < blockDim.x) { s[index] += s[index + stride]; }
        __syncthreads();
    }
    if (tid == 0) { out[blockIdx.x] = s[0]; }
}

// v3: sequential addressing: the active threads read consecutive words
template <typename T>
__global__ void reduceV3(const T* in, T* out, int n)
{
    __shared__ T s[BLOCK];
    unsigned tid = threadIdx.x;
    unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
    s[tid] = (i < static_cast<unsigned>(n)) ? in[i] : T(0);
    __syncthreads();
    for (unsigned stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (tid < stride) { s[tid] += s[tid + stride]; }
        __syncthreads();
    }
    if (tid == 0) { out[blockIdx.x] = s[0]; }
}

// v4: first add during the load: each block covers 2 * BLOCK elements, half the blocks
template <typename T>
__global__ void reduceV4(const T* in, T* out, int n)
{
    __shared__ T s[BLOCK];
    unsigned tid = threadIdx.x;
    size_t i = static_cast<size_t>(blockIdx.x) * (2 * blockDim.x) + threadIdx.x;
    T v = (i < static_cast<size_t>(n)) ? in[i] : T(0);
    if (i + blockDim.x < static_cast<size_t>(n)) { v += in[i + blockDim.x]; }
    s[tid] = v;
    __syncthreads();
    for (unsigned stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (tid < stride) { s[tid] += s[tid + stride]; }
        __syncthreads();
    }
    if (tid == 0) { out[blockIdx.x] = s[0]; }
}

template <typename T>
__device__ T warpSum(T v)
{
    for (int offset = warpSize / 2; offset > 0; offset /= 2) {   // no hard-coded 32
        v += __shfl_down_sync(0xffffffffu, v, offset);
    }
    return v;                                                      // lane 0 holds the warp's sum
}

template <typename T>
__device__ T blockSum(T v)
{
    __shared__ T warpTotals[BLOCK / 32];          // assumes warps of 32 for the array size only
    int lane = threadIdx.x % warpSize;
    int warp = threadIdx.x / warpSize;
    v = warpSum(v);
    if (lane == 0) { warpTotals[warp] = v; }
    __syncthreads();
    int warps = blockDim.x / warpSize;
    v = (threadIdx.x < static_cast<unsigned>(warps)) ? warpTotals[lane] : T(0);
    if (warp == 0) { v = warpSum(v); }
    return v;                                      // thread 0 holds the block's sum
}

// v5: first add during load + shuffles instead of shared-memory steps
template <typename T>
__global__ void reduceV5(const T* in, T* out, int n)
{
    size_t i = static_cast<size_t>(blockIdx.x) * (2 * blockDim.x) + threadIdx.x;
    T v = (i < static_cast<size_t>(n)) ? in[i] : T(0);
    if (i + blockDim.x < static_cast<size_t>(n)) { v += in[i + blockDim.x]; }
    v = blockSum(v);
    if (threadIdx.x == 0) { out[blockIdx.x] = v; }
}

// v6: a fixed grid walks the whole input (grid-stride loop), then one atomicAdd per block:
// the complete sum in ONE launch.
template <typename T>
__global__ void reduceV6(const T* in, T* total, size_t n)
{
    T v = T(0);
    for (size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        v += in[i];
    }
    v = blockSum(v);
    if (threadIdx.x == 0) { atomicAdd(total, v); }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// Runs version v (1..5: per-block partials, finished by repeated launches; 6: one launch).
template <typename T>
T runVersion(int v, const T* dIn, int n, T* dA, T* dB, int grid6)
{
    const T* src = dIn;
    int count = n;
    T* dst = dA;
    if (v == 6) {
        check(cudaMemset(dA, 0, sizeof(T)), "memset total");
        reduceV6<T><<<grid6, BLOCK>>>(dIn, dA, static_cast<size_t>(n));
        check(cudaGetLastError(), "reduceV6");
    } else {
        while (count > 1) {
            int per = (v >= 4) ? 2 * BLOCK : BLOCK;
            int blocks = (count + per - 1) / per;
            if (v == 1) { reduceV1<T><<<blocks, BLOCK>>>(src, dst, count); }
            if (v == 2) { reduceV2<T><<<blocks, BLOCK>>>(src, dst, count); }
            if (v == 3) { reduceV3<T><<<blocks, BLOCK>>>(src, dst, count); }
            if (v == 4) { reduceV4<T><<<blocks, BLOCK>>>(src, dst, count); }
            if (v == 5) { reduceV5<T><<<blocks, BLOCK>>>(src, dst, count); }
            check(cudaGetLastError(), "reduce launch");
            src = dst;
            dst = (dst == dA) ? dB : dA;
            count = blocks;
        }
        dA = const_cast<T*>(src);
    }
    T result{};
    check(cudaMemcpy(&result, dA, sizeof(T), cudaMemcpyDeviceToHost), "copy result");
    return result;
}

template <typename T>
float medianMs(int v, const T* dIn, int n, T* dA, T* dB, int grid6)
{
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "event");
    check(cudaEventCreate(&stop), "event");
    std::vector<float> ms(20);
    runVersion<T>(v, dIn, n, dA, dB, grid6);                       // warm-up
    for (float& m : ms) {
        check(cudaEventRecord(start), "record");
        runVersion<T>(v, dIn, n, dA, dB, grid6);
        check(cudaEventRecord(stop), "record");
        check(cudaEventSynchronize(stop), "sync");
        check(cudaEventElapsedTime(&m, start, stop), "elapsed");
    }
    std::sort(ms.begin(), ms.end());
    check(cudaEventDestroy(start), "event");
    check(cudaEventDestroy(stop), "event");
    return (ms[9] + ms[10]) / 2;
}

int main()
{
    int dev = 0, sms = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");                  // fail early without a GPU
    check(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, dev), "SM count");
    const int n = 1 << 28;                                          // E3: 2^28 elements
    std::vector<float> hf(n);
    std::vector<int> hi(n);
    double kahanSum = 0.0, c = 0.0, absSum = 0.0;
    long long intSum = 0;
    for (int i = 0; i < n; ++i) {
        unsigned h = static_cast<unsigned>(i) * 2654435761u;
        hf[i] = static_cast<float>(h % 1000) / 1000.0f;
        hi[i] = static_cast<int>(h % 4);
        double y = hf[i] - c, t = kahanSum + y;                    // Kahan summation (MA302, F0-75)
        c = (t - kahanSum) - y;
        kahanSum = t;
        absSum += std::fabs(hf[i]);
        intSum += hi[i];
    }
    float *dF = nullptr, *dFA = nullptr, *dFB = nullptr;
    int *dI = nullptr, *dIA = nullptr, *dIB = nullptr;
    const int partials = (n + BLOCK - 1) / BLOCK;
    check(cudaMalloc(&dF, sizeof(float) * n), "malloc");
    check(cudaMalloc(&dI, sizeof(int) * n), "malloc");
    check(cudaMalloc(&dFA, sizeof(float) * partials), "malloc");
    check(cudaMalloc(&dFB, sizeof(float) * partials), "malloc");
    check(cudaMalloc(&dIA, sizeof(int) * partials), "malloc");
    check(cudaMalloc(&dIB, sizeof(int) * partials), "malloc");
    check(cudaMemcpy(dF, hf.data(), sizeof(float) * n, cudaMemcpyHostToDevice), "copy");
    check(cudaMemcpy(dI, hi.data(), sizeof(int) * n, cudaMemcpyHostToDevice), "copy");
    const int grid6 = sms * 8;                                      // a fixed grid for v6 (tunable)
    // error bound for float sums of non-negative data: (terms added in sequence + tree depth) * eps * sum|x|
    const double perThread = std::ceil(static_cast<double>(n) / (static_cast<double>(grid6) * BLOCK));
    const double tol = (perThread + 2.0 * std::log2(static_cast<double>(n))) * 1.1920929e-7 * absSum;
    std::printf("n = %d, int reference %lld, float reference (Kahan, double) %.3f, tolerance %.3f\n",
                n, intSum, kahanSum, tol);
    std::printf("%-4s %-14s %-6s %-16s %-6s %-10s %s\n", "ver", "int sum", "exact", "float sum", "ok",
                "ms (med)", "GB/s");
    for (int v = 1; v <= 6; ++v) {
        int gi = runVersion<int>(v, dI, n, dIA, dIB, grid6);
        float gf = runVersion<float>(v, dF, n, dFA, dFB, grid6);
        float ms = medianMs<float>(v, dF, n, dFA, dFB, grid6);
        double gbs = static_cast<double>(n) * sizeof(float) / (ms * 1.0e6);
        std::printf("v%-3d %-14d %-6s %-16.3f %-6s %-10.3f %.1f\n", v, gi, gi == intSum ? "yes" : "NO", gf,
                    std::fabs(gf - kahanSum) <= tol ? "yes" : "NO", ms, gbs);
    }
    for (void* p : {static_cast<void*>(dF), static_cast<void*>(dI), static_cast<void*>(dFA),
                    static_cast<void*>(dFB), static_cast<void*>(dIA), static_cast<void*>(dIB)}) {
        check(cudaFree(p), "cudaFree");
    }
    return 0;
}
