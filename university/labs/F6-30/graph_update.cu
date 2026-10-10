// F6-30 Listing 4: the fix for the forensic lab. A graph whose kernel argument
// (the gain) changes every frame: capture the frame's work again, then try to
// update the executable graph in place; instantiate anew only if the update fails.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void applyGain(float* frame, int n, float gain)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        frame[i] *= gain;
    }
}

// Records one frame's work into `graph` by stream capture (nothing runs here).
void captureFrame(cudaStream_t s, float* dFrame, const float* hFrame, int n, float gain,
                  cudaGraph_t* graph)
{
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    CUDA_CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    CUDA_CHECK(cudaMemcpyAsync(dFrame, hFrame, bytes, cudaMemcpyHostToDevice, s));
    applyGain<<<(n + 255) / 256, 256, 0, s>>>(dFrame, n, gain);   // gain copied now
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaStreamEndCapture(s, graph));
}

int main()
{
    const int n = 1 << 16;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const float gains[4] = {1.0f, 2.0f, 4.0f, 0.5f};
    float* hFrame = nullptr;                                 // pinned: copies may be async
    float* dFrame = nullptr;
    CUDA_CHECK(cudaMallocHost(&hFrame, bytes));
    CUDA_CHECK(cudaMalloc(&dFrame, bytes));
    for (int i = 0; i < n; ++i) {
        hFrame[i] = 10.0f;
    }
    cudaStream_t s = nullptr;
    CUDA_CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    cudaGraphExec_t exec = nullptr;
    std::vector<cudaGraph_t> graphs;                         // destroyed at the end
    int updated = 0;
    int instantiated = 0;
    for (int f = 0; f < 4; ++f) {
        cudaGraph_t graph = nullptr;
        captureFrame(s, dFrame, hFrame, n, gains[f], &graph);
        graphs.push_back(graph);
        if (exec == nullptr) {
            CUDA_CHECK(cudaGraphInstantiate(&exec, graph, 0));
            ++instantiated;
        } else {
            cudaGraphExecUpdateResultInfo info{};
            if (cudaGraphExecUpdate(exec, graph, &info) == cudaSuccess) {
                ++updated;                                   // same shape: new parameters
            } else {
                CUDA_CHECK(cudaGetLastError());              // clear, then rebuild
                CUDA_CHECK(cudaGraphExecDestroy(exec));
                CUDA_CHECK(cudaGraphInstantiate(&exec, graph, 0));
                ++instantiated;
            }
        }
        CUDA_CHECK(cudaGraphLaunch(exec, s));
        float pixel = 0.0f;
        CUDA_CHECK(cudaMemcpyAsync(&pixel, dFrame, sizeof(float), cudaMemcpyDeviceToHost, s));
        CUDA_CHECK(cudaStreamSynchronize(s));
        std::printf("frame %d: gain %.1f, pixel %.1f (expected %.1f)\n", f, gains[f], pixel,
                    10.0f * gains[f]);
    }
    std::printf("instantiated %d time(s), updated in place %d time(s)\n", instantiated, updated);
    CUDA_CHECK(cudaGraphExecDestroy(exec));
    for (cudaGraph_t g : graphs) {
        CUDA_CHECK(cudaGraphDestroy(g));
    }
    CUDA_CHECK(cudaStreamDestroy(s));
    CUDA_CHECK(cudaFree(dFrame));
    CUDA_CHECK(cudaFreeHost(hFrame));
    return 0;
}
