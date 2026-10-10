// MP5 starter, Listing 7: the GPU pieces of one ring step, for the hardware run of milestone 1.
// reduceInto adds a received chunk into the owner's segment (phase 0 of the ring); the host
// part checks it against the CPU for awkward sizes and prints which device pairs report peer
// access (the links the ring will use, F8-04). In this build there is no GPU, so the run
// stops at device discovery: untested on hardware.
#include <cstdio>
#include <cuda_runtime.h>
#include <vector>

__global__ void reduceInto(float* dst, const float* src, size_t n)
{
    for (size_t i = blockIdx.x * static_cast<size_t>(blockDim.x) + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        dst[i] += src[i];
    }
}

static bool check(cudaError_t e, const char* what)
{
    if (e != cudaSuccess) {
        std::printf("%s failed: %s (%s)\n", what, cudaGetErrorName(e), cudaGetErrorString(e));
        return false;
    }
    return true;
}

int main()
{
    int devices = 0;
    if (!check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount")) {
        return 1;
    }
    std::printf("devices: %d\n", devices);
    for (int a = 0; a < devices; ++a) {
        for (int b = 0; b < devices; ++b) {
            int can = 0;
            if (a != b && check(cudaDeviceCanAccessPeer(&can, a, b), "cudaDeviceCanAccessPeer")) {
                std::printf("peer access %d -> %d: %s\n", a, b, can ? "yes" : "no");
            }
        }
    }
    bool ok = true;
    for (size_t n : {size_t{1}, size_t{1000}, (size_t{1} << 20) + 3}) {
        std::vector<float> dst(n), src(n), out(n);
        for (size_t i = 0; i < n; ++i) {
            dst[i] = static_cast<float>(i % 97) * 0.5f;
            src[i] = static_cast<float>(i % 89) * 0.25f;   // every sum exact in float
        }
        float* dDst = nullptr;
        float* dSrc = nullptr;
        ok = check(cudaMalloc(&dDst, n * sizeof(float)), "cudaMalloc") && ok;
        ok = check(cudaMalloc(&dSrc, n * sizeof(float)), "cudaMalloc") && ok;
        if (!ok) {
            return 1;
        }
        check(cudaMemcpy(dDst, dst.data(), n * sizeof(float), cudaMemcpyHostToDevice), "cudaMemcpy");
        check(cudaMemcpy(dSrc, src.data(), n * sizeof(float), cudaMemcpyHostToDevice), "cudaMemcpy");
        reduceInto<<<256, 256>>>(dDst, dSrc, n);
        ok = check(cudaGetLastError(), "launch") && check(cudaDeviceSynchronize(), "sync") && ok;
        check(cudaMemcpy(out.data(), dDst, n * sizeof(float), cudaMemcpyDeviceToHost), "cudaMemcpy");
        size_t bad = 0;
        for (size_t i = 0; i < n; ++i) {
            bad += out[i] != dst[i] + src[i] ? 1 : 0;
        }
        std::printf("reduceInto n=%zu: %s\n", n, bad == 0 ? "PASS" : "FAIL");
        ok = ok && bad == 0;
        cudaFree(dDst);
        cudaFree(dSrc);
    }
    return ok ? 0 : 1;
}
