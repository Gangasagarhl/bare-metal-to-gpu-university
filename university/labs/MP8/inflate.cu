// inflate.cu - MP8 starter lab: the perception kernel in CUDA (the MP3 side of the example).
// One thread per output cell runs mp8::inflateCell, the same rule the CPU stand-in runs;
// the result is compared with the learner's F9-54 reference on the host.
// Built for real; untested on hardware (the build container has no GPU, AH-26).
#include "../F6-05/cuda_check.h"
#include "perception.hpp"
#include <cstdio>
#include <vector>

__global__ void inflateKernel(const std::uint8_t* occ, int width, int height, const int* offs, int nOffs,
                              std::uint8_t* out)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const int j = blockIdx.y * blockDim.y + threadIdx.y;
    if (i < width && j < height) {
        out[j * width + i] = mp8::inflateCell(occ, width, height, offs, nOffs, i, j);
    }
}

int main()
{
    const rb::Grid house = rb::makeHouse({{3.0, 1.4, 3.3, 1.9}});
    const std::vector<std::uint8_t> occ = mp8::occBytes(house);
    const std::vector<int> offs = mp8::discOffsets(0.25);
    const std::vector<std::uint8_t> want = mp8::inflateReference(house, 0.25);
    const int n = rb::kW * rb::kH;
    const int nOffs = static_cast<int>(offs.size() / 2);
    std::printf("map %d x %d cells, %d disc offsets, reference computed on the CPU\n", rb::kW, rb::kH, nOffs);
    std::fflush(stdout);   // keep this line before any error message in the saved output

    std::uint8_t* dOcc = nullptr;
    std::uint8_t* dOut = nullptr;
    int* dOffs = nullptr;
    CUDA_CHECK(cudaMalloc(&dOcc, n));
    CUDA_CHECK(cudaMalloc(&dOut, n));
    CUDA_CHECK(cudaMalloc(&dOffs, offs.size() * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(dOcc, occ.data(), n, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dOffs, offs.data(), offs.size() * sizeof(int), cudaMemcpyHostToDevice));
    const dim3 block(16, 16);
    const dim3 grid((rb::kW + block.x - 1) / block.x, (rb::kH + block.y - 1) / block.y);
    inflateKernel<<<grid, block>>>(dOcc, rb::kW, rb::kH, dOffs, nOffs, dOut);
    CUDA_CHECK_LAUNCH();
    std::vector<std::uint8_t> got(n);
    CUDA_CHECK(cudaMemcpy(got.data(), dOut, n, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(dOcc));
    CUDA_CHECK(cudaFree(dOut));
    CUDA_CHECK(cudaFree(dOffs));
    int differ = 0;
    for (int k = 0; k < n; ++k) {
        differ += (got[k] != want[k]) ? 1 : 0;
    }
    std::printf("device result vs F9-54 reference: %d cells differ\n", differ);
    return differ == 0 ? 0 : 1;
}
