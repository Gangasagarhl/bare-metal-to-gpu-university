// F7-17 one_mfma: a single MFMA builtin, compiled for several targets (compile only).
#include <hip/hip_runtime.h>

typedef _Float16 half4 __attribute__((ext_vector_type(4)));
typedef float float16v __attribute__((ext_vector_type(16)));

__global__ void oneMfma(const half4* a, const half4* b, float16v* c)
{
    float16v acc = c[threadIdx.x];
    acc = __builtin_amdgcn_mfma_f32_32x32x8f16(a[threadIdx.x], b[threadIdx.x], acc, 0, 0, 0);
    c[threadIdx.x] = acc;
}
