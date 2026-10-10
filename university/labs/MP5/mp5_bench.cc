// MP5 starter, Listing 5: the bandwidth-curve harness for milestone 1, on CPU threads.
// Persistent threads (one per "GPU"), one warm-up call, then the median of 20 timed calls per
// size, timed by rank 0 between two barriers. Prints, per size: messages and bytes sent per
// rank (the model's inputs), the median time, algbw = S / t and busbw = algbw * 2(N-1)/N,
// the nccl-tests convention for all-reduce (F8-03). mp5_fit.py fits alpha and beta to this.
// Built with -O2 and no sanitizers by run.sh, because sanitizers change timing.
#include "mp5_allreduce.h"

#include <algorithm>
#include <barrier>
#include <cstdio>
#include <thread>

namespace {

constexpr int kReps = 20;
constexpr std::size_t kChunk = 16384;   // elements per chunk (64 KiB of float)

double medianUs(std::size_t n, std::size_t count)
{
    std::vector<std::vector<float>> buf(n, std::vector<float>(count, 1.0f));
    mp5::ThreadRing<float> ring(n, std::chrono::milliseconds(60000));
    std::vector<mp5::ThreadRing<float>::ThreadLink> links;
    links.reserve(n);
    for (std::size_t r = 0; r < n; ++r) {
        links.emplace_back(ring, r);
    }
    std::barrier sync(static_cast<std::ptrdiff_t>(n));
    std::vector<double> times;
    bool failed = false;
    std::vector<std::thread> threads;
    for (std::size_t r = 0; r < n; ++r) {
        threads.emplace_back([&, r] {
            for (int rep = 0; rep <= kReps; ++rep) {          // rep 0 is the warm-up
                std::fill(buf[r].begin(), buf[r].end(), 1.0f);
                sync.arrive_and_wait();
                const auto t0 = std::chrono::steady_clock::now();
                const mp5::Status st = mp5::ringAllReduce<float>(links[r], r, n, std::span<float>(buf[r]), kChunk);
                sync.arrive_and_wait();
                if (r == 0) {
                    const auto t1 = std::chrono::steady_clock::now();
                    if (rep > 0) {
                        times.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
                    }
                }
                if (st != mp5::Status::ok) {
                    failed = true;
                }
            }
        });
    }
    for (std::thread& t : threads) {
        t.join();
    }
    for (std::size_t r = 0; r < n && !failed; ++r) {
        failed = buf[r][count / 2] != static_cast<float>(n);   // every element must be n
    }
    if (failed) {
        return -1.0;
    }
    std::sort(times.begin(), times.end());
    return (times[kReps / 2 - 1] + times[kReps / 2]) / 2.0;
}

}  // namespace

int main()
{
    std::printf("# CPU threads stand in for GPUs; float sum; chunk %zu elements; median of %d calls after 1 warm-up\n",
                kChunk, kReps);
    std::printf("# %2s %10s %9s %12s %12s %10s %10s\n", "N", "bytes", "msgs", "bytes_sent", "median_us", "algbw_GBs", "busbw_GBs");
    bool ok = true;
    for (std::size_t n : {2u, 4u}) {
        for (std::size_t bytes = 4; bytes <= (std::size_t{64} << 20); bytes *= 4) {
            const std::size_t count = bytes / sizeof(float);
            const std::size_t maxSeg = (count + n - 1) / n;
            const std::size_t chunks = std::max<std::size_t>(1, (maxSeg + kChunk - 1) / kChunk);
            const std::size_t msgs = 2 * (n - 1) * chunks;                       // per rank
            const double sent = 2.0 * static_cast<double>(n - 1) / static_cast<double>(n) * static_cast<double>(bytes);
            const double us = medianUs(n, count);
            if (us < 0) {
                ok = false;
                std::printf("  %2zu %10zu FAILED\n", n, bytes);
                continue;
            }
            const double algbw = static_cast<double>(bytes) / (us * 1e3);          // GB/s: bytes per ns
            const double busbw = algbw * 2.0 * static_cast<double>(n - 1) / static_cast<double>(n);
            std::printf("  %2zu %10zu %9zu %12.0f %12.2f %10.4f %10.4f\n", n, bytes, msgs, sent, us, algbw, busbw);
        }
    }
    std::printf("# result: %s\n", ok ? "all timed calls correct" : "FAILURES");
    return ok ? 0 : 1;
}
