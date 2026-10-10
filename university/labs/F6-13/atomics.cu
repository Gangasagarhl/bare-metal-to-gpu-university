// F6-13 Listing 1: counting the elements that pass a test, four ways.
//   countPlain      - (*counter)++            : the forensic bug (a read-modify-write that is not atomic)
//   countAtomic     - atomicAdd per thread    : correct; every passing thread hits one address
//   countWarpAgg    - one atomicAdd per warp  : correct; ballot + popc, the leader adds the warp's total
//   countBlockAgg   - shared-memory atomics, then one global atomicAdd per block
// Plus atomicMaxFloat, a compare-and-swap loop that builds an atomic operation the hardware lacks.
// Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

__device__ bool passes(int v) { return v % 3 == 0; }

__global__ void countPlain(const int* in, int n, int* counter)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && passes(in[i])) {
        (*counter)++;                                   // load, add, store: NOT atomic
    }
}

__global__ void countAtomic(const int* in, int n, int* counter)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && passes(in[i])) {
        atomicAdd(counter, 1);
    }
}

__global__ void countWarpAgg(const int* in, int n, int* counter)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    bool p = i < n && passes(in[i]);
    unsigned mask = __ballot_sync(0xffffffffu, p);      // bit k set = lane k passes
    int lane = threadIdx.x % 32;
    if (mask != 0 && lane == __ffs(mask) - 1) {         // lowest passing lane is the leader
        atomicAdd(counter, __popc(mask));               // one atomic for the whole warp
    }
}

__global__ void countBlockAgg(const int* in, int n, int* counter)
{
    __shared__ int blockCount;
    if (threadIdx.x == 0) { blockCount = 0; }
    __syncthreads();
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && passes(in[i])) {
        atomicAdd(&blockCount, 1);                      // shared-memory atomic
    }
    __syncthreads();
    if (threadIdx.x == 0) { atomicAdd(counter, blockCount); }
}

__device__ float atomicMaxFloat(float* addr, float value)
{
    int* bits = reinterpret_cast<int*>(addr);
    int old = *bits;
    while (__int_as_float(old) < value) {
        int assumed = old;
        old = atomicCAS(bits, assumed, __float_as_int(value));   // succeeds only if nobody changed it
        if (old == assumed) { break; }
    }
    return __int_as_float(old);
}

__global__ void maxKernel(const int* in, int n, float* result)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) { atomicMaxFloat(result, static_cast<float>(in[i])); }
}

__global__ void scopes(int* counter)
{
#if __CUDA_ARCH__ >= 600
    atomicAdd_block(counter, 1);       // atomic with respect to this block's threads only
    atomicAdd_system(counter + 1, 1);  // atomic with respect to the host and other devices too
#endif
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
    const int n = 1 << 24, threads = 256, blocks = (n + threads - 1) / threads;
    std::vector<int> h(n);
    int expect = 0, hmax = 0;
    for (int i = 0; i < n; ++i) {
        h[i] = (i * 2654435761u) % 1000003;
        expect += (h[i] % 3 == 0);
        hmax = h[i] > hmax ? h[i] : hmax;
    }
    int* dIn = nullptr;
    int* dCount = nullptr;
    float* dMax = nullptr;
    check(cudaMalloc(&dIn, sizeof(int) * n), "cudaMalloc in");
    check(cudaMalloc(&dCount, sizeof(int)), "cudaMalloc count");
    check(cudaMalloc(&dMax, sizeof(float)), "cudaMalloc max");
    check(cudaMemcpy(dIn, h.data(), sizeof(int) * n, cudaMemcpyHostToDevice), "copy in");
    const char* names[] = {"countPlain", "countAtomic", "countWarpAgg", "countBlockAgg"};
    for (int v = 0; v < 4; ++v) {
        check(cudaMemset(dCount, 0, sizeof(int)), "memset count");
        if (v == 0) { countPlain<<<blocks, threads>>>(dIn, n, dCount); }
        if (v == 1) { countAtomic<<<blocks, threads>>>(dIn, n, dCount); }
        if (v == 2) { countWarpAgg<<<blocks, threads>>>(dIn, n, dCount); }
        if (v == 3) { countBlockAgg<<<blocks, threads>>>(dIn, n, dCount); }
        check(cudaGetLastError(), names[v]);
        int got = 0;
        check(cudaMemcpy(&got, dCount, sizeof got, cudaMemcpyDeviceToHost), "copy count");
        std::printf("%-14s %d (expected %d)\n", names[v], got, expect);
    }
    float zero = 0.0f, gotMax = 0.0f;
    check(cudaMemcpy(dMax, &zero, sizeof zero, cudaMemcpyHostToDevice), "init max");
    maxKernel<<<blocks, threads>>>(dIn, n, dMax);
    check(cudaGetLastError(), "maxKernel");
    check(cudaMemcpy(&gotMax, dMax, sizeof gotMax, cudaMemcpyDeviceToHost), "copy max");
    std::printf("atomicMaxFloat %.0f (expected %d)\n", gotMax, hmax);
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dCount), "cudaFree count");
    check(cudaFree(dMax), "cudaFree max");
    return 0;
}
