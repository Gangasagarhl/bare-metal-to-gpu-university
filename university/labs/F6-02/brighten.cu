// F6-02 Listing 1: a two-dimensional launch. Each thread brightens one pixel of a
// width x height greyscale image stored row by row (row-major) in one array.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

__global__ void brighten(unsigned char* img, int width, int height, int amount)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;  // column
    int y = blockIdx.y * blockDim.y + threadIdx.y;  // row
    if (x < width && y < height) {
        int i = y * width + x;                      // row-major position
        int v = img[i] + amount;
        img[i] = static_cast<unsigned char>(v > 255 ? 255 : v);
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int width = 1000;
    const int height = 600;
    const std::size_t bytes = static_cast<std::size_t>(width) * height;
    std::vector<unsigned char> hImg(bytes);
    for (std::size_t i = 0; i < bytes; ++i) { hImg[i] = static_cast<unsigned char>(i % 200); }

    unsigned char* dImg = nullptr;
    check(cudaMalloc(&dImg, bytes), "cudaMalloc image");
    check(cudaMemcpy(dImg, hImg.data(), bytes, cudaMemcpyHostToDevice), "copy image to device");

    const dim3 block(16, 16);                                  // 256 threads per block
    const dim3 grid((width + block.x - 1) / block.x,           // round up in x
                    (height + block.y - 1) / block.y);         // and in y
    std::printf("grid %u x %u blocks of %u x %u threads\n", grid.x, grid.y, block.x, block.y);
    brighten<<<grid, block>>>(dImg, width, height, 40);
    check(cudaGetLastError(), "kernel launch");

    std::vector<unsigned char> out(bytes);
    check(cudaMemcpy(out.data(), dImg, bytes, cudaMemcpyDeviceToHost), "copy image to host");
    int errors = 0;
    for (std::size_t i = 0; i < bytes; ++i) {
        int want = hImg[i] + 40;
        if (out[i] != (want > 255 ? 255 : want)) { ++errors; }
    }
    std::printf("checked %zu pixels, %d errors\n", bytes, errors);
    check(cudaFree(dImg), "cudaFree image");
    return errors == 0 ? 0 : 1;
}
