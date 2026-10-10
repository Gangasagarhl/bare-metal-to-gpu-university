// F1-61 Listing 2: one wavefront issues one AMD matrix instruction (MFMA) through a
// compiler builtin. 16x16x4, FP32 inputs and FP32 accumulators. Compiled for gfx90a;
// run.sh shows the instruction in the assembly. Untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

typedef float floatx4 __attribute__((ext_vector_type(4)));

__global__ void mfmaOnce(const float* a, const float* b, float* c)
{
    floatx4 acc = {0.0f, 0.0f, 0.0f, 0.0f};
    // each of the 64 lanes supplies one element of A and one of B, and receives 4 results
    acc = __builtin_amdgcn_mfma_f32_16x16x4f32(a[threadIdx.x], b[threadIdx.x], acc, 0, 0, 0);
    for (int k = 0; k < 4; ++k) {
        c[threadIdx.x * 4 + k] = acc[k];
    }
}

int main()
{
    float* p = nullptr;
    hipError_t err = hipMalloc(&p, 384 * sizeof(float));
    if (err != hipSuccess) {
        std::fprintf(stderr, "hipMalloc failed: %s (%s)\n", hipGetErrorName(err), hipGetErrorString(err));
        return EXIT_FAILURE;
    }
    mfmaOnce<<<1, 64>>>(p, p + 64, p + 128);
    err = hipDeviceSynchronize();
    std::printf("kernel finished: %s\n", hipGetErrorString(err));
    (void)hipFree(p);
    return err == hipSuccess ? 0 : 1;
}
