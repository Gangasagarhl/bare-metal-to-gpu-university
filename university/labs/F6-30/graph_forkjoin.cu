// F6-30 Listing 2: capturing a fork and a join. Two branches run in two streams
// during capture; the events become edges of the graph. The graph is written as a
// DOT file so you can look at its shape.
#include <cstdio>
#include "../F6-05/cuda_check.h"

__global__ void setTo(float* x, int n, float v)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = v;
    }
}

__global__ void addInto(float* out, const float* a, const float* b, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = a[i] + b[i];
    }
}

int main()
{
    const int n = 1 << 16;
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    CUDA_CHECK(cudaMalloc(&dA, bytes));
    CUDA_CHECK(cudaMalloc(&dB, bytes));
    CUDA_CHECK(cudaMalloc(&dC, bytes));
    cudaStream_t mainStream = nullptr;
    cudaStream_t side = nullptr;
    CUDA_CHECK(cudaStreamCreateWithFlags(&mainStream, cudaStreamNonBlocking));
    CUDA_CHECK(cudaStreamCreateWithFlags(&side, cudaStreamNonBlocking));
    cudaEvent_t forkEv = nullptr;
    cudaEvent_t joinEv = nullptr;
    CUDA_CHECK(cudaEventCreateWithFlags(&forkEv, cudaEventDisableTiming));
    CUDA_CHECK(cudaEventCreateWithFlags(&joinEv, cudaEventDisableTiming));

    cudaGraph_t graph = nullptr;
    CUDA_CHECK(cudaStreamBeginCapture(mainStream, cudaStreamCaptureModeGlobal));
    CUDA_CHECK(cudaEventRecord(forkEv, mainStream));
    CUDA_CHECK(cudaStreamWaitEvent(side, forkEv, 0));     // side joins the capture
    setTo<<<blocks, threads, 0, mainStream>>>(dA, n, 1.0f);     // branch 1
    CUDA_CHECK_LAUNCH();
    setTo<<<blocks, threads, 0, side>>>(dB, n, 2.0f);     // branch 2
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(joinEv, side));
    CUDA_CHECK(cudaStreamWaitEvent(mainStream, joinEv, 0));     // join before the sum
    addInto<<<blocks, threads, 0, mainStream>>>(dC, dA, dB, n);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaStreamEndCapture(mainStream, &graph));

    std::size_t nodes = 0;
    std::size_t edges = 0;
    CUDA_CHECK(cudaGraphGetNodes(graph, nullptr, &nodes));
    CUDA_CHECK(cudaGraphGetEdges(graph, nullptr, nullptr, &edges));
    CUDA_CHECK(cudaGraphDebugDotPrint(graph, "forkjoin.dot", 0));
    std::printf("captured graph: %zu nodes, %zu edges (written to forkjoin.dot)\n", nodes, edges);

    cudaGraphExec_t exec = nullptr;
    CUDA_CHECK(cudaGraphInstantiate(&exec, graph, 0));
    CUDA_CHECK(cudaGraphLaunch(exec, mainStream));
    float c0 = 0.0f;
    CUDA_CHECK(cudaMemcpyAsync(&c0, dC, sizeof(float), cudaMemcpyDeviceToHost, mainStream));
    CUDA_CHECK(cudaStreamSynchronize(mainStream));
    std::printf("C[0] = %.1f (expected 3.0)\n", c0);

    CUDA_CHECK(cudaGraphExecDestroy(exec));
    CUDA_CHECK(cudaGraphDestroy(graph));
    CUDA_CHECK(cudaEventDestroy(forkEv));
    CUDA_CHECK(cudaEventDestroy(joinEv));
    CUDA_CHECK(cudaStreamDestroy(mainStream));
    CUDA_CHECK(cudaStreamDestroy(side));
    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));
    return c0 == 3.0f ? 0 : 1;
}
