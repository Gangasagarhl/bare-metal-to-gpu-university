// inflate_hip.hip - MP8 starter lab: the same perception kernel in HIP (the MP4 side).
// Same rule (mp8::inflateCell), same host reference (F9-54); error checks in the style of F7-05.
// Built for real for gfx90a; untested on hardware (the build container has no GPU, AH-26).
#include <hip/hip_runtime.h>
#include "perception.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(1);
    }
}

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
    check(hipMalloc(&dOcc, n), "hipMalloc occ");
    check(hipMalloc(&dOut, n), "hipMalloc out");
    check(hipMalloc(&dOffs, offs.size() * sizeof(int)), "hipMalloc offs");
    check(hipMemcpy(dOcc, occ.data(), n, hipMemcpyHostToDevice), "hipMemcpy occ");
    check(hipMemcpy(dOffs, offs.data(), offs.size() * sizeof(int), hipMemcpyHostToDevice), "hipMemcpy offs");
    const dim3 block(16, 16);
    const dim3 grid((rb::kW + block.x - 1) / block.x, (rb::kH + block.y - 1) / block.y);
    inflateKernel<<<grid, block>>>(dOcc, rb::kW, rb::kH, dOffs, nOffs, dOut);
    check(hipGetLastError(), "launch");
    std::vector<std::uint8_t> got(n);
    check(hipMemcpy(got.data(), dOut, n, hipMemcpyDeviceToHost), "hipMemcpy out");
    check(hipFree(dOcc), "hipFree occ");
    check(hipFree(dOut), "hipFree out");
    check(hipFree(dOffs), "hipFree offs");
    int differ = 0;
    for (int k = 0; k < n; ++k) {
        differ += (got[k] != want[k]) ? 1 : 0;
    }
    std::printf("device result vs F9-54 reference: %d cells differ\n", differ);
    return differ == 0 ? 0 : 1;
}
