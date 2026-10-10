// F6-34 forensic program: each thread keeps 16 running sums of one row.
// The two SASS listings of the forensic lab come from this one file, built twice.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void rowSums16(const float* __restrict__ in, float* __restrict__ out, int rows, int cols)
{
    int r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r >= rows) {
        return;
    }
    float acc[16];
#pragma unroll
    for (int j = 0; j < 16; ++j) {
        acc[j] = 0.0f;
    }
    for (int c = 0; c < cols; c += 16) {
#pragma unroll
        for (int j = 0; j < 16; ++j) {
            float v = in[static_cast<long long>(r) * cols + c + j];
            acc[j] = acc[j] * 0.99f + v * v;
        }
    }
    float s = 0.0f;
#pragma unroll
    for (int j = 0; j < 16; ++j) {
        s += acc[j];
    }
    out[r] = s;
}

int main()
{
    const int rows = 1 << 14;
    const int cols = 256;                                 // a multiple of 16
    const std::size_t count = static_cast<std::size_t>(rows) * cols;
    std::vector<float> h(count, 1.0f);
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&dOut, static_cast<std::size_t>(rows) * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(dIn, h.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    rowSums16<<<(rows + 127) / 128, 128>>>(dIn, dOut, rows, cols);
    CUDA_CHECK_LAUNCH();
    float r0 = 0.0f;
    CUDA_CHECK(cudaMemcpy(&r0, dOut, sizeof(float), cudaMemcpyDeviceToHost));
    std::printf("row 0 result %.4f\n", r0);
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    return 0;
}
