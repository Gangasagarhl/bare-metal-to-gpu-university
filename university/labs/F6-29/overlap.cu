// F6-29 Listing 1: a chunked copy-in / compute / copy-out pipeline (milestone E8).
//   1. Time each stage alone on the whole array, in one stream (pinned memory).
//   2. Predict the pipelined time from those three times (formula of Listing 2).
//   3. Run the pipeline: nChunks chunks spread round-robin over nStreams streams.
//   4. Run it again from pageable host memory.
//   5. Check both pipelined results bit for bit against the one-stream result.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void process(const float* in, float* out, int n, int reps)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        float v = in[i];
        for (int r = 0; r < reps; ++r) {
            v = v * 0.999f + 0.001f;                      // enough work to be visible
        }
        out[i] = v;
    }
}

struct Timer                                              // two events in one stream
{
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    Timer()
    {
        CUDA_CHECK(cudaEventCreate(&start));
        CUDA_CHECK(cudaEventCreate(&stop));
    }
    ~Timer()
    {
        cudaEventDestroy(start);                          // destructor: no exit on error
        cudaEventDestroy(stop);
    }
    float ms()
    {
        CUDA_CHECK(cudaEventSynchronize(stop));
        float t = 0.0f;
        CUDA_CHECK(cudaEventElapsedTime(&t, start, stop));
        return t;
    }
};

// Runs the pipeline from hIn to hOut and returns its time in ms. Stream 0 of the
// array forks the work: every stream waits for `fork`; stream 0 joins on all ends.
float pipeline(const float* hIn, float* hOut, float* dIn, float* dOut, int n, int reps,
               std::vector<cudaStream_t>& streams, int nChunks)
{
    const int nStreams = static_cast<int>(streams.size());
    const int chunk = (n + nChunks - 1) / nChunks;
    const int threads = 256;
    Timer t;
    cudaEvent_t fork = nullptr;
    CUDA_CHECK(cudaEventCreateWithFlags(&fork, cudaEventDisableTiming));
    std::vector<cudaEvent_t> done(static_cast<std::size_t>(nStreams));
    for (cudaEvent_t& e : done) {
        CUDA_CHECK(cudaEventCreateWithFlags(&e, cudaEventDisableTiming));
    }
    CUDA_CHECK(cudaEventRecord(t.start, streams[0]));
    CUDA_CHECK(cudaEventRecord(fork, streams[0]));
    for (std::size_t s = 1; s < streams.size(); ++s) {
        CUDA_CHECK(cudaStreamWaitEvent(streams[s], fork, 0));
    }
    for (int c = 0; c < nChunks; ++c) {
        const int first = c * chunk;
        const int count = std::min(chunk, n - first);
        if (count <= 0) {
            break;
        }
        const std::size_t off = static_cast<std::size_t>(first);
        const std::size_t bytes = static_cast<std::size_t>(count) * sizeof(float);
        cudaStream_t s = streams[static_cast<std::size_t>(c % nStreams)];
        CUDA_CHECK(cudaMemcpyAsync(dIn + off, hIn + off, bytes, cudaMemcpyHostToDevice, s));
        const int blocks = (count + threads - 1) / threads;
        process<<<blocks, threads, 0, s>>>(dIn + off, dOut + off, count, reps);
        CUDA_CHECK_LAUNCH();
        CUDA_CHECK(cudaMemcpyAsync(hOut + off, dOut + off, bytes, cudaMemcpyDeviceToHost, s));
    }
    for (std::size_t s = 1; s < streams.size(); ++s) {
        CUDA_CHECK(cudaEventRecord(done[s], streams[s]));
        CUDA_CHECK(cudaStreamWaitEvent(streams[0], done[s], 0));
    }
    CUDA_CHECK(cudaEventRecord(t.stop, streams[0]));
    const float ms = t.ms();
    for (cudaEvent_t& e : done) {
        CUDA_CHECK(cudaEventDestroy(e));
    }
    CUDA_CHECK(cudaEventDestroy(fork));
    return ms;
}

int main()
{
    const int n = 1 << 24;                                // 64 MiB of floats
    const int reps = 200;
    const int nStreams = 4;
    const int nChunks = 8;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const int threads = 256;

    float* hIn = nullptr;                                 // pinned (page-locked) buffers
    float* hOut = nullptr;
    float* hRef = nullptr;
    CUDA_CHECK(cudaMallocHost(&hIn, bytes));
    CUDA_CHECK(cudaMallocHost(&hOut, bytes));
    CUDA_CHECK(cudaMallocHost(&hRef, bytes));
    for (int i = 0; i < n; ++i) {
        hIn[i] = static_cast<float>(i % 1000) * 0.001f;
    }
    std::vector<float> pageIn(hIn, hIn + n);              // pageable copies of the same data
    std::vector<float> pageOut(static_cast<std::size_t>(n), 0.0f);
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, bytes));
    CUDA_CHECK(cudaMalloc(&dOut, bytes));
    std::vector<cudaStream_t> streams(static_cast<std::size_t>(nStreams));
    for (cudaStream_t& s : streams) {
        CUDA_CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    }

    // 1. Each stage alone, whole array, one stream (also produces the reference).
    cudaStream_t s0 = streams[0];
    Timer tIn;
    Timer tKernel;
    Timer tOut;
    CUDA_CHECK(cudaMemsetAsync(dIn, 0, bytes, s0));       // defined input for the warm-up
    process<<<(n + threads - 1) / threads, threads, 0, s0>>>(dIn, dOut, n, reps);   // warm-up
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(tIn.start, s0));
    CUDA_CHECK(cudaMemcpyAsync(dIn, hIn, bytes, cudaMemcpyHostToDevice, s0));
    CUDA_CHECK(cudaEventRecord(tIn.stop, s0));
    CUDA_CHECK(cudaEventRecord(tKernel.start, s0));
    process<<<(n + threads - 1) / threads, threads, 0, s0>>>(dIn, dOut, n, reps);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(tKernel.stop, s0));
    CUDA_CHECK(cudaEventRecord(tOut.start, s0));
    CUDA_CHECK(cudaMemcpyAsync(hRef, dOut, bytes, cudaMemcpyDeviceToHost, s0));
    CUDA_CHECK(cudaEventRecord(tOut.stop, s0));
    const float a = tIn.ms();
    const float k = tKernel.ms();
    const float b = tOut.ms();

    // 2. Prediction for two copy engines, equal chunks, no per-chunk cost.
    const float serial = a + k + b;
    const float predicted = serial / nChunks + (nChunks - 1) * std::max({a, k, b}) / nChunks;

    // 3. and 4. The pipeline from pinned and from pageable memory.
    const float pinnedMs = pipeline(hIn, hOut, dIn, dOut, n, reps, streams, nChunks);
    const float pageableMs =
        pipeline(pageIn.data(), pageOut.data(), dIn, dOut, n, reps, streams, nChunks);

    // 5. Same kernel, same GPU, same inputs: the results must be identical.
    const bool okPinned = std::memcmp(hOut, hRef, bytes) == 0;
    const bool okPageable = std::memcmp(pageOut.data(), hRef, bytes) == 0;

    std::printf("stage times (ms): copy in %.3f, kernel %.3f, copy out %.3f, serial %.3f\n",
                a, k, b, serial);
    std::printf("pipeline %d chunks, %d streams: predicted %.3f ms\n", nChunks, nStreams,
                predicted);
    std::printf("measured pinned   %.3f ms (%.2f of predicted), result %s\n", pinnedMs,
                pinnedMs / predicted, okPinned ? "identical" : "DIFFERENT");
    std::printf("measured pageable %.3f ms (%.2f of predicted), result %s\n", pageableMs,
                pageableMs / predicted, okPageable ? "identical" : "DIFFERENT");

    for (cudaStream_t& s : streams) {
        CUDA_CHECK(cudaStreamDestroy(s));
    }
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    CUDA_CHECK(cudaFreeHost(hIn));
    CUDA_CHECK(cudaFreeHost(hOut));
    CUDA_CHECK(cudaFreeHost(hRef));
    return (okPinned && okPageable) ? 0 : 1;
}
