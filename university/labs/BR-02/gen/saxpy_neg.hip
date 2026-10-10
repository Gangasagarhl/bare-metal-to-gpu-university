// BR-02 Listing 1 (starter, CUDA): the SAXPY of F6-04 (milestone E1) plus one statistic,
// "how many results are negative", counted with a warp vote. The 32-lane assumption on
// lines 21 and 22 is PLANTED on purpose: it is correct on NVIDIA GPUs and is the bug
// this bridge's lab finds and fixes after the port to HIP.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <hip/hip_runtime.h>

__global__ void saxpyNeg(int n, float a, const float* x, float* y, unsigned int* negatives)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    bool negative = false;
    if (i < n) {
        y[i] = a * x[i] + y[i];
        negative = y[i] < 0.0f;
    }
    const unsigned int mask = __ballot_sync(0xffffffffu, negative);  // one bit per lane
    if ((threadIdx.x & 31) == 0) {                                   // lane 0 of each warp
        atomicAdd(negatives, static_cast<unsigned int>(__popc(mask)));
    }
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err),
                     hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

static std::uint32_t bits(float f)
{
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

// The same deterministic test data as F6-04 (no library random generator).
static float value(std::uint32_t i, std::uint32_t seed)
{
    std::uint32_t h = (i + seed) * 2654435761u;
    h ^= h >> 15;
    return static_cast<float>(h % 2000003u) / 1000.0f - 1000.0f;
}

int main()
{
    int devices = 0;
    check(hipGetDeviceCount(&devices), "hipGetDeviceCount");
    const float a = 1.7f;
    const int sizes[] = {1, 7, 255, 256, 257, 1000, 1 << 20, 1000003, 100000000};
    int failures = 0;
    for (int n : sizes) {
        std::vector<float> x(n), y(n), ref(n);
        unsigned int refNeg = 0;
        for (int i = 0; i < n; ++i) {
            x[i] = value(i, 1);
            y[i] = value(i, 2);
            ref[i] = std::fma(a, x[i], y[i]);  // one rounding, as in F6-04
            refNeg += ref[i] < 0.0f ? 1u : 0u;
        }
        const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
        float* dX = nullptr;
        float* dY = nullptr;
        unsigned int* dNeg = nullptr;
        check(hipMalloc(&dX, bytes), "hipMalloc x");
        check(hipMalloc(&dY, bytes), "hipMalloc y");
        check(hipMalloc(&dNeg, sizeof(unsigned int)), "hipMalloc negatives");
        check(hipMemcpy(dX, x.data(), bytes, hipMemcpyHostToDevice), "copy x");
        check(hipMemcpy(dY, y.data(), bytes, hipMemcpyHostToDevice), "copy y");
        check(hipMemset(dNeg, 0, sizeof(unsigned int)), "hipMemset negatives");
        const int threads = 256;
        const int blocks = (n + threads - 1) / threads;
        saxpyNeg<<<blocks, threads>>>(n, a, dX, dY, dNeg);
        check(hipGetLastError(), "saxpyNeg launch");
        unsigned int neg = 0;
        check(hipMemcpy(y.data(), dY, bytes, hipMemcpyDeviceToHost), "copy y back");
        check(hipMemcpy(&neg, dNeg, sizeof neg, hipMemcpyDeviceToHost), "copy negatives");
        check(hipFree(dX), "hipFree x");
        check(hipFree(dY), "hipFree y");
        check(hipFree(dNeg), "hipFree negatives");
        int mismatches = 0;
        for (int i = 0; i < n; ++i) {
            if (bits(y[i]) != bits(ref[i])) { ++mismatches; }
        }
        std::printf("n = %9d  bit mismatches = %d  negatives = %u (reference %u)\n", n,
                    mismatches, neg, refNeg);
        if (mismatches != 0 || neg != refNeg) { ++failures; }
    }
    std::printf("%s\n", failures == 0 ? "PASS: bit-exact and counts equal" : "FAIL");
    return failures == 0 ? 0 : 1;
}
