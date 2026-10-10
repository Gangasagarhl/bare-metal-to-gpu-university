// F8-05 Listing 2: the ring of Listing 1 on real GPUs, one host thread driving all of them.
// Rank r = GPU r. Each GPU has its buffer and two receive slots of one chunk each. A "tick"
// is one chunk of one step: every GPU copies a chunk to its right neighbour's slot (copy
// stream), then every GPU adds (or, in the all-gather, copies) the chunk in its own slot
// into its buffer (compute stream). Events order the work across GPUs without stopping the
// host: a copy waits until the chunk it sends is final and until the slot it overwrites has
// been consumed; an add waits until its chunk has arrived and its own sends have finished.
// Built for real with nvcc; untested on hardware (the build container has no GPU).
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

__global__ void addInto(float* dst, const float* src, size_t n)
{
    for (size_t i = blockIdx.x * static_cast<size_t>(blockDim.x) + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        dst[i] += src[i];
    }
}

__global__ void copyInto(float* dst, const float* src, size_t n)
{
    for (size_t i = blockIdx.x * static_cast<size_t>(blockDim.x) + threadIdx.x; i < n;
         i += static_cast<size_t>(gridDim.x) * blockDim.x) {
        dst[i] = src[i];
    }
}

struct Range
{
    size_t begin;
    size_t end;
};

static Range chunkRange(size_t count, size_t n, size_t g, size_t k, size_t chunks)
{
    const size_t s0 = g * count / n;
    const size_t len = (g + 1) * count / n - s0;
    return {s0 + k * len / chunks, s0 + (k + 1) * len / chunks};
}

// all-reduce (sum) of d_buf[r], count floats on every GPU r; chunkElems floats per chunk
static void ringAllReduce(const std::vector<float*>& d_buf, size_t count, size_t chunkElems)
{
    const size_t n = d_buf.size();
    const size_t maxSeg = (count + n - 1) / n;
    const size_t chunks = maxSeg == 0 ? 1 : (maxSeg + chunkElems - 1) / chunkElems;
    const size_t ticks = 2 * (n - 1) * chunks;
    std::vector<float*> d_slot(2 * n);
    std::vector<cudaStream_t> copyS(n), compS(n);
    std::vector<std::vector<cudaEvent_t>> copied(n, std::vector<cudaEvent_t>(ticks));
    std::vector<std::vector<cudaEvent_t>> consumed(n, std::vector<cudaEvent_t>(ticks));
    for (size_t r = 0; r < n; ++r) {
        check(cudaSetDevice(static_cast<int>(r)), "cudaSetDevice");
        check(cudaMalloc(&d_slot[2 * r], chunkElems * sizeof(float)), "cudaMalloc slot 0");
        check(cudaMalloc(&d_slot[2 * r + 1], chunkElems * sizeof(float)), "cudaMalloc slot 1");
        check(cudaStreamCreate(&copyS[r]), "cudaStreamCreate copy");
        check(cudaStreamCreate(&compS[r]), "cudaStreamCreate compute");
        for (size_t t = 0; t < ticks; ++t) {
            check(cudaEventCreateWithFlags(&copied[r][t], cudaEventDisableTiming), "cudaEventCreate");
            check(cudaEventCreateWithFlags(&consumed[r][t], cudaEventDisableTiming), "cudaEventCreate");
        }
    }
    for (size_t t = 0; t < ticks; ++t) {
        const size_t phase = t / ((n - 1) * chunks);        // 0: reduce-scatter, 1: all-gather
        const size_t step = (t / chunks) % (n - 1);
        const size_t k = t % chunks;
        const size_t slot = t % 2;
        for (size_t r = 0; r < n; ++r) {                     // 1. every GPU sends one chunk right
            const size_t next = (r + 1) % n;
            const Range s = chunkRange(count, n, (r + 2 * n - step + phase) % n, k, chunks);
            check(cudaSetDevice(static_cast<int>(r)), "cudaSetDevice");
            if (t >= chunks) {                               // the chunk was finished at tick t - chunks
                check(cudaStreamWaitEvent(copyS[r], consumed[r][t - chunks], 0), "wait chunk final");
            }
            if (t >= 2) {                                    // the neighbour emptied this slot at t - 2
                check(cudaStreamWaitEvent(copyS[r], consumed[next][t - 2], 0), "wait slot free");
            }
            if (s.end > s.begin) {
                check(cudaMemcpyPeerAsync(d_slot[2 * next + slot], static_cast<int>(next), d_buf[r] + s.begin,
                                          static_cast<int>(r), (s.end - s.begin) * sizeof(float), copyS[r]),
                      "cudaMemcpyPeerAsync");
            }
            check(cudaEventRecord(copied[r][t], copyS[r]), "cudaEventRecord copied");
        }
        for (size_t r = 0; r < n; ++r) {                     // 2. every GPU uses the chunk it got
            const size_t prev = (r + n - 1) % n;
            const Range d = chunkRange(count, n, (r + 2 * n - step - 1 + phase) % n, k, chunks);
            check(cudaSetDevice(static_cast<int>(r)), "cudaSetDevice");
            check(cudaStreamWaitEvent(compS[r], copied[prev][t], 0), "wait chunk arrived");
            // and until this GPU's own earlier sends finished reading its buffer
            check(cudaStreamWaitEvent(compS[r], copied[r][t], 0), "wait own sends done");
            if (d.end > d.begin) {
                const size_t len = d.end - d.begin;
                const unsigned blocks = static_cast<unsigned>((len + 255) / 256 < 1024 ? (len + 255) / 256 : 1024);
                if (phase == 0) {
                    addInto<<<blocks, 256, 0, compS[r]>>>(d_buf[r] + d.begin, d_slot[2 * r + slot], len);
                } else {
                    copyInto<<<blocks, 256, 0, compS[r]>>>(d_buf[r] + d.begin, d_slot[2 * r + slot], len);
                }
                check(cudaGetLastError(), "kernel launch");
            }
            check(cudaEventRecord(consumed[r][t], compS[r]), "cudaEventRecord consumed");
        }
    }
    for (size_t r = 0; r < n; ++r) {
        check(cudaSetDevice(static_cast<int>(r)), "cudaSetDevice");
        check(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
        for (size_t t = 0; t < ticks; ++t) {
            check(cudaEventDestroy(copied[r][t]), "cudaEventDestroy");
            check(cudaEventDestroy(consumed[r][t]), "cudaEventDestroy");
        }
        check(cudaStreamDestroy(copyS[r]), "cudaStreamDestroy");
        check(cudaStreamDestroy(compS[r]), "cudaStreamDestroy");
        check(cudaFree(d_slot[2 * r]), "cudaFree");
        check(cudaFree(d_slot[2 * r + 1]), "cudaFree");
    }
}

int main()
{
    int devices = 0;
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    if (devices < 2) {
        std::printf("%d GPU(s): a ring needs at least 2\n", devices);
        return 0;
    }
    bool allOk = true;
    for (int n = 2; n <= devices; n *= 2) {
        for (size_t bytes : {size_t{4}, size_t{1028}, size_t{1} << 20, size_t{256} << 20}) {
            const size_t count = bytes / sizeof(float);
            std::vector<float*> d_buf(static_cast<size_t>(n));
            std::vector<float> h(count);
            for (int r = 0; r < n; ++r) {                    // integer values: every sum is exact
                for (size_t i = 0; i < count; ++i) {
                    h[i] = static_cast<float>((r + 1) * static_cast<int>(i % 251));
                }
                check(cudaSetDevice(r), "cudaSetDevice");
                check(cudaMalloc(&d_buf[static_cast<size_t>(r)], bytes), "cudaMalloc");
                check(cudaMemcpy(d_buf[static_cast<size_t>(r)], h.data(), bytes, cudaMemcpyHostToDevice), "copy in");
            }
            ringAllReduce(d_buf, count, size_t{1} << 18);    // 1 MiB chunks
            size_t wrong = 0;
            for (int r = 0; r < n; ++r) {
                check(cudaSetDevice(r), "cudaSetDevice");
                check(cudaMemcpy(h.data(), d_buf[static_cast<size_t>(r)], bytes, cudaMemcpyDeviceToHost), "copy out");
                for (size_t i = 0; i < count; ++i) {
                    wrong += h[i] != static_cast<float>(n * (n + 1) / 2 * static_cast<int>(i % 251));
                }
                check(cudaFree(d_buf[static_cast<size_t>(r)]), "cudaFree");
            }
            std::printf("N=%d %10zu bytes: %s\n", n, bytes, wrong == 0 ? "PASS" : "FAIL");
            allOk = allOk && wrong == 0;
        }
    }
    return allOk ? 0 : 1;
}
