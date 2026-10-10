// F6-30 Listing 1: 100 small kernels, launched one by one and as one CUDA graph
// (milestone E8, second acceptance test). The graph is made by stream capture.
#include <cstdio>
#include <cstring>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void step(float* x, int n, float a)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = x[i] * a + 1.0f;
    }
}

// Issues the whole sequence into `stream`: kernel k multiplies by 1 + k / 1000.
void issueSequence(float* dX, int n, int kernels, cudaStream_t stream)
{
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    for (int k = 0; k < kernels; ++k) {
        step<<<blocks, threads, 0, stream>>>(dX, n, 1.0f + static_cast<float>(k) / 1000.0f);
        CUDA_CHECK_LAUNCH();
    }
}

int main()
{
    const int n = 1 << 12;                                // small: launch cost dominates
    const int kernels = 100;
    const int repeats = 20;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    std::vector<float> hInit(static_cast<std::size_t>(n), 0.5f);
    std::vector<float> hLoose(static_cast<std::size_t>(n));
    std::vector<float> hGraph(static_cast<std::size_t>(n));
    float* dX = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    // A. Individual launches: `repeats` times the 100-kernel sequence.
    CUDA_CHECK(cudaMemcpyAsync(dX, hInit.data(), bytes, cudaMemcpyHostToDevice, stream));
    issueSequence(dX, n, kernels, stream);                // warm-up (not timed)
    CUDA_CHECK(cudaMemcpyAsync(dX, hInit.data(), bytes, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaEventRecord(start, stream));
    for (int r = 0; r < repeats; ++r) {
        issueSequence(dX, n, kernels, stream);
    }
    CUDA_CHECK(cudaEventRecord(stop, stream));
    CUDA_CHECK(cudaEventSynchronize(stop));
    float looseMs = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&looseMs, start, stop));
    CUDA_CHECK(cudaMemcpyAsync(hLoose.data(), dX, bytes, cudaMemcpyDeviceToHost, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));

    // B. Capture the same sequence once, instantiate once, launch `repeats` times.
    cudaGraph_t graph = nullptr;
    cudaGraphExec_t graphExec = nullptr;
    CUDA_CHECK(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal));
    issueSequence(dX, n, kernels, stream);                // recorded, not executed
    CUDA_CHECK(cudaStreamEndCapture(stream, &graph));
    std::size_t nodes = 0;
    CUDA_CHECK(cudaGraphGetNodes(graph, nullptr, &nodes));
    CUDA_CHECK(cudaGraphInstantiate(&graphExec, graph, 0));
    CUDA_CHECK(cudaGraphLaunch(graphExec, stream));       // warm-up launch (not timed)
    CUDA_CHECK(cudaMemcpyAsync(dX, hInit.data(), bytes, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaEventRecord(start, stream));
    for (int r = 0; r < repeats; ++r) {
        CUDA_CHECK(cudaGraphLaunch(graphExec, stream));
    }
    CUDA_CHECK(cudaEventRecord(stop, stream));
    CUDA_CHECK(cudaEventSynchronize(stop));
    float graphMs = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&graphMs, start, stop));
    CUDA_CHECK(cudaMemcpyAsync(hGraph.data(), dX, bytes, cudaMemcpyDeviceToHost, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));

    const bool same = std::memcmp(hLoose.data(), hGraph.data(), bytes) == 0;
    const int launches = kernels * repeats;
    std::printf("graph nodes: %zu\n", nodes);
    std::printf("individual launches: %.3f ms for %d kernels (%.2f us per kernel)\n", looseMs,
                launches, 1000.0f * looseMs / launches);
    std::printf("graph launches:      %.3f ms for %d kernels (%.2f us per kernel)\n", graphMs,
                launches, 1000.0f * graphMs / launches);
    std::printf("saving per kernel: %.2f us; results %s\n",
                1000.0f * (looseMs - graphMs) / launches,
                same ? "identical" : "DIFFERENT");

    CUDA_CHECK(cudaGraphExecDestroy(graphExec));
    CUDA_CHECK(cudaGraphDestroy(graph));
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaStreamDestroy(stream));
    CUDA_CHECK(cudaFree(dX));
    return same ? 0 : 1;
}
