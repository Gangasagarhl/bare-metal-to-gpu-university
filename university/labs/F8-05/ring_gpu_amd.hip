// F8-05 Listing 3: Listing 2 in HIP for AMD GPUs, made by renaming every cuda* name to hip*.
// Rank r = GPU r. Each GPU has its buffer and two receive slots of one chunk each. A "tick"
// is one chunk of one step: every GPU copies a chunk to its right neighbour's slot (copy
// stream), then every GPU adds (or, in the all-gather, copies) the chunk in its own slot
// into its buffer (compute stream). Events order the work across GPUs without stopping the
// host: a copy waits until the chunk it sends is final and until the slot it overwrites has
// been consumed; an add waits until its chunk has arrived and its own sends have finished.
// Built for real with hipcc; untested on hardware (the build container has no GPU).
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
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
    std::vector<hipStream_t> copyS(n), compS(n);
    std::vector<std::vector<hipEvent_t>> copied(n, std::vector<hipEvent_t>(ticks));
    std::vector<std::vector<hipEvent_t>> consumed(n, std::vector<hipEvent_t>(ticks));
    for (size_t r = 0; r < n; ++r) {
        check(hipSetDevice(static_cast<int>(r)), "hipSetDevice");
        check(hipMalloc(&d_slot[2 * r], chunkElems * sizeof(float)), "hipMalloc slot 0");
        check(hipMalloc(&d_slot[2 * r + 1], chunkElems * sizeof(float)), "hipMalloc slot 1");
        check(hipStreamCreate(&copyS[r]), "hipStreamCreate copy");
        check(hipStreamCreate(&compS[r]), "hipStreamCreate compute");
        for (size_t t = 0; t < ticks; ++t) {
            check(hipEventCreateWithFlags(&copied[r][t], hipEventDisableTiming), "hipEventCreate");
            check(hipEventCreateWithFlags(&consumed[r][t], hipEventDisableTiming), "hipEventCreate");
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
            check(hipSetDevice(static_cast<int>(r)), "hipSetDevice");
            if (t >= chunks) {                               // the chunk was finished at tick t - chunks
                check(hipStreamWaitEvent(copyS[r], consumed[r][t - chunks], 0), "wait chunk final");
            }
            if (t >= 2) {                                    // the neighbour emptied this slot at t - 2
                check(hipStreamWaitEvent(copyS[r], consumed[next][t - 2], 0), "wait slot free");
            }
            if (s.end > s.begin) {
                check(hipMemcpyPeerAsync(d_slot[2 * next + slot], static_cast<int>(next), d_buf[r] + s.begin,
                                          static_cast<int>(r), (s.end - s.begin) * sizeof(float), copyS[r]),
                      "hipMemcpyPeerAsync");
            }
            check(hipEventRecord(copied[r][t], copyS[r]), "hipEventRecord copied");
        }
        for (size_t r = 0; r < n; ++r) {                     // 2. every GPU uses the chunk it got
            const size_t prev = (r + n - 1) % n;
            const Range d = chunkRange(count, n, (r + 2 * n - step - 1 + phase) % n, k, chunks);
            check(hipSetDevice(static_cast<int>(r)), "hipSetDevice");
            check(hipStreamWaitEvent(compS[r], copied[prev][t], 0), "wait chunk arrived");
            // and until this GPU's own earlier sends finished reading its buffer
            check(hipStreamWaitEvent(compS[r], copied[r][t], 0), "wait own sends done");
            if (d.end > d.begin) {
                const size_t len = d.end - d.begin;
                const unsigned blocks = static_cast<unsigned>((len + 255) / 256 < 1024 ? (len + 255) / 256 : 1024);
                if (phase == 0) {
                    addInto<<<blocks, 256, 0, compS[r]>>>(d_buf[r] + d.begin, d_slot[2 * r + slot], len);
                } else {
                    copyInto<<<blocks, 256, 0, compS[r]>>>(d_buf[r] + d.begin, d_slot[2 * r + slot], len);
                }
                check(hipGetLastError(), "kernel launch");
            }
            check(hipEventRecord(consumed[r][t], compS[r]), "hipEventRecord consumed");
        }
    }
    for (size_t r = 0; r < n; ++r) {
        check(hipSetDevice(static_cast<int>(r)), "hipSetDevice");
        check(hipDeviceSynchronize(), "hipDeviceSynchronize");
        for (size_t t = 0; t < ticks; ++t) {
            check(hipEventDestroy(copied[r][t]), "hipEventDestroy");
            check(hipEventDestroy(consumed[r][t]), "hipEventDestroy");
        }
        check(hipStreamDestroy(copyS[r]), "hipStreamDestroy");
        check(hipStreamDestroy(compS[r]), "hipStreamDestroy");
        check(hipFree(d_slot[2 * r]), "hipFree");
        check(hipFree(d_slot[2 * r + 1]), "hipFree");
    }
}

int main()
{
    int devices = 0;
    check(hipGetDeviceCount(&devices), "hipGetDeviceCount");
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
                check(hipSetDevice(r), "hipSetDevice");
                check(hipMalloc(&d_buf[static_cast<size_t>(r)], bytes), "hipMalloc");
                check(hipMemcpy(d_buf[static_cast<size_t>(r)], h.data(), bytes, hipMemcpyHostToDevice), "copy in");
            }
            ringAllReduce(d_buf, count, size_t{1} << 18);    // 1 MiB chunks
            size_t wrong = 0;
            for (int r = 0; r < n; ++r) {
                check(hipSetDevice(r), "hipSetDevice");
                check(hipMemcpy(h.data(), d_buf[static_cast<size_t>(r)], bytes, hipMemcpyDeviceToHost), "copy out");
                for (size_t i = 0; i < count; ++i) {
                    wrong += h[i] != static_cast<float>(n * (n + 1) / 2 * static_cast<int>(i % 251));
                }
                check(hipFree(d_buf[static_cast<size_t>(r)]), "hipFree");
            }
            std::printf("N=%d %10zu bytes: %s\n", n, bytes, wrong == 0 ? "PASS" : "FAIL");
            allOk = allOk && wrong == 0;
        }
    }
    return allOk ? 0 : 1;
}
