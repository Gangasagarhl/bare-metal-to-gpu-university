// F7-17 wmma_rdna3: the RDNA3 matrix instruction through its builtin, wave32 (compile only).
#include <hip/hip_runtime.h>

typedef _Float16 half16 __attribute__((ext_vector_type(16)));
typedef float float8v __attribute__((ext_vector_type(8)));

__global__ void oneWmma(const half16* a, const half16* b, float8v* c)
{
    float8v acc = c[threadIdx.x];
    acc = __builtin_amdgcn_wmma_f32_16x16x16_f16_w32(a[threadIdx.x], b[threadIdx.x], acc);
    c[threadIdx.x] = acc;
}
