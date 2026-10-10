// F6-32 Listing 1: the F6-29 pipeline idea with NVTX ranges, so that a timeline
// tool can show the program's own phases next to the GPU work. NVTX calls cost
// close to nothing when no tool is attached (installed header, nvToolsExt.h).
#include <cstdio>
#include <vector>
#include <nvtx3/nvToolsExt.h>
#include "../F6-05/cuda_check.h"

__global__ void process(const float* in, float* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i] * in[i] + 1.0f;
    }
}

int main(int argc, char** argv)
{
    const bool usePinned = !(argc > 1 && argv[1][0] == 'p');   // "p" = pageable
    const int n = 1 << 24;
    const int nChunks = 8;
    const int nStreams = 4;
    const int chunk = n / nChunks;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const int threads = 256;

    nvtxRangePushA("allocate");
    std::vector<float> pageIn;
    std::vector<float> pageOut;
    float* hIn = nullptr;
    float* hOut = nullptr;
    if (usePinned) {
        CUDA_CHECK(cudaMallocHost(&hIn, bytes));
        CUDA_CHECK(cudaMallocHost(&hOut, bytes));
    } else {
        pageIn.resize(static_cast<std::size_t>(n));
        pageOut.resize(static_cast<std::size_t>(n));
        hIn = pageIn.data();
        hOut = pageOut.data();
    }
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, bytes));
    CUDA_CHECK(cudaMalloc(&dOut, bytes));
    std::vector<cudaStream_t> streams(static_cast<std::size_t>(nStreams));
    for (cudaStream_t& s : streams) {
        CUDA_CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    }
    nvtxRangePop();

    nvtxRangePushA("fill input");
    for (int i = 0; i < n; ++i) {
        hIn[i] = static_cast<float>(i % 64);
    }
    nvtxRangePop();

    nvtxRangePushA(usePinned ? "pipeline (pinned)" : "pipeline (pageable)");
    for (int c = 0; c < nChunks; ++c) {
        const std::size_t off = static_cast<std::size_t>(c) * static_cast<std::size_t>(chunk);
        const std::size_t cb = static_cast<std::size_t>(chunk) * sizeof(float);
        cudaStream_t s = streams[static_cast<std::size_t>(c % nStreams)];
        CUDA_CHECK(cudaMemcpyAsync(dIn + off, hIn + off, cb, cudaMemcpyHostToDevice, s));
        process<<<(chunk + threads - 1) / threads, threads, 0, s>>>(dIn + off, dOut + off, chunk);
        CUDA_CHECK_LAUNCH();
        CUDA_CHECK(cudaMemcpyAsync(hOut + off, dOut + off, cb, cudaMemcpyDeviceToHost, s));
    }
    CUDA_CHECK(cudaDeviceSynchronize());
    nvtxRangePop();

    nvtxRangePushA("check");
    long long errors = 0;
    for (int i = 0; i < n; ++i) {
        const float v = static_cast<float>(i % 64);
        if (hOut[i] != v * v + 1.0f) {
            ++errors;
        }
    }
    nvtxRangePop();
    std::printf("%s: %lld errors\n", usePinned ? "pinned" : "pageable", errors);

    for (cudaStream_t& s : streams) {
        CUDA_CHECK(cudaStreamDestroy(s));
    }
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    if (usePinned) {
        CUDA_CHECK(cudaFreeHost(hIn));
        CUDA_CHECK(cudaFreeHost(hOut));
    }
    return errors == 0 ? 0 : 1;
}
