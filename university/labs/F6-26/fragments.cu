// F6-26 Listing 4: how many elements does one thread hold in a WMMA fragment? Host code that
// reads the compile-time constant num_elements from the toolkit's mma.h (no GPU needed).
// A 16 x 16 tile has 256 elements and a warp has 32 threads, so 8 per thread would be an even
// split; any other number tells you the layout is not a simple split.
#include <cstdio>
#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <mma.h>

// Host pass only: the device pass of a default build targets an architecture for which mma.h
// declares no fragments, and this program has no device code anyway.
#if !defined(__CUDA_ARCH__)
using namespace nvcuda;

template <typename Frag>
void show(const char* name, int rows, int cols)
{
    std::printf("%-44s tile %2d x %2d = %3d elements; per thread %2d; x 32 threads = %3d\n", name,
                rows, cols, rows * cols, int(Frag::num_elements), 32 * int(Frag::num_elements));
}

int main()
{
    show<wmma::fragment<wmma::matrix_a, 16, 16, 16, __half, wmma::row_major>>(
        "matrix_a 16x16x16 half", 16, 16);
    show<wmma::fragment<wmma::matrix_b, 16, 16, 16, __half, wmma::row_major>>(
        "matrix_b 16x16x16 half", 16, 16);
    show<wmma::fragment<wmma::matrix_a, 16, 16, 16, __nv_bfloat16, wmma::row_major>>(
        "matrix_a 16x16x16 __nv_bfloat16", 16, 16);
    show<wmma::fragment<wmma::accumulator, 16, 16, 16, float>>("accumulator 16x16x16 float", 16, 16);
    show<wmma::fragment<wmma::accumulator, 16, 16, 16, __half>>("accumulator 16x16x16 half", 16, 16);
    show<wmma::fragment<wmma::matrix_a, 32, 8, 16, __half, wmma::row_major>>(
        "matrix_a 32x8x16 half (A is 32 x 16)", 32, 16);
    show<wmma::fragment<wmma::matrix_a, 16, 16, 8, wmma::precision::tf32, wmma::row_major>>(
        "matrix_a 16x16x8 tf32 (A is 16 x 8)", 16, 8);
    return 0;
}
#endif
