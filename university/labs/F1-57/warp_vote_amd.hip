// F1-57 Listing 3: the same kernel in HIP for AMD GPUs. The vote returns a 64-bit
// mask (one bit per lane of a wavefront). Compiled for gfx90a; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__global__ void collatzStep(const int* in, int* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) {
        return;
    }
    int v = in[i];
    if (v & 1) {
        v = v * 3 + 1;
    } else {
        v = v / 2;
    }
    unsigned long long votes = __ballot(v > 10);           // 64-bit lane mask
    out[i] = v + __popcll(votes) + warpSize;
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1024;
    std::vector<int> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = i; }
    int* dIn = nullptr;
    int* dOut = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc in");
    check(hipMalloc(&dOut, n * sizeof(int)), "hipMalloc out");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "copy in");
    collatzStep<<<n / 256, 256>>>(dIn, dOut, n);
    check(hipGetLastError(), "kernel launch");
    check(hipMemcpy(hOut.data(), dOut, n * sizeof(int), hipMemcpyDeviceToHost), "copy out");
    std::printf("out[0..3] = %d %d %d %d\n", hOut[0], hOut[1], hOut[2], hOut[3]);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dOut), "hipFree out");
    return 0;
}
