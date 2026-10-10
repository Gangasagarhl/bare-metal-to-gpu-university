// BR-03 Listing 1: a 256-bin histogram of bytes on the CPU with std::thread, three ways.
//   serial        one thread; the reference every other version is compared with
//   sharedAtomic  every thread clicks one shared table of std::atomic counters
//   privateMerge  every thread counts in its own table; the main thread adds the tables
// Each parallel version can split the input "chunked" (thread t gets one contiguous slice)
// or "interleaved" (thread t gets bytes t, t+T, t+2T, ...).
// No argument: correctness run on 2^20 bytes (the run_lab.sh build, with sanitizers).
// Argument "time": medians of 5 runs on 2^24 bytes and OS evidence (run.sh, -O2, no sanitizers).
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <pthread.h>
#include <sys/resource.h>

constexpr int kBins = 256;
using Bins = std::array<unsigned long, kBins>;

std::vector<unsigned char> makeInput(std::size_t n, bool skewed)
{
    std::vector<unsigned char> in(n);
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned r = static_cast<unsigned>(i) * 2654435761u;  // a cheap, repeatable scramble
        in[i] = (skewed && r % 10 != 0) ? 0 : static_cast<unsigned char>(r >> 24);
    }
    return in;
}

Bins histSerial(const std::vector<unsigned char>& in)
{
    Bins bins{};
    for (unsigned char b : in) {
        ++bins[b];
    }
    return bins;
}

// Calls f(i) for every byte index i that thread t of T is responsible for.
template <typename F>
void forMyBytes(std::size_t n, unsigned t, unsigned T, bool interleaved, F f)
{
    if (interleaved) {
        for (std::size_t i = t; i < n; i += T) {
            f(i);
        }
    } else {
        const std::size_t begin = n * t / T;
        const std::size_t end = n * (t + 1) / T;
        for (std::size_t i = begin; i < end; ++i) {
            f(i);
        }
    }
}

Bins histSharedAtomic(const std::vector<unsigned char>& in, unsigned T, bool interleaved)
{
    std::array<std::atomic<unsigned long>, kBins> shared{};  // C++20: every counter starts at 0
    std::vector<std::thread> cooks;
    for (unsigned t = 0; t < T; ++t) {
        cooks.emplace_back([&in, &shared, t, T, interleaved] {
            forMyBytes(in.size(), t, T, interleaved, [&](std::size_t i) {
                shared[in[i]].fetch_add(1, std::memory_order_relaxed);  // one click, never lost
            });
        });
    }
    for (std::thread& c : cooks) {
        c.join();
    }
    Bins bins{};
    for (int b = 0; b < kBins; ++b) {
        bins[b] = shared[b].load();
    }
    return bins;
}

Bins histPrivateMerge(const std::vector<unsigned char>& in, unsigned T, bool interleaved)
{
    std::vector<Bins> tables(T);
    std::vector<std::thread> cooks;
    for (unsigned t = 0; t < T; ++t) {
        cooks.emplace_back([&in, &tables, t, T, interleaved] {
            Bins mine{};  // private table on this thread's own stack: no sharing while counting
            forMyBytes(in.size(), t, T, interleaved, [&](std::size_t i) { ++mine[in[i]]; });
            tables[t] = mine;  // one write per thread, at the end
        });
    }
    for (std::thread& c : cooks) {
        c.join();  // join() orders every table write before the merge below
    }
    Bins bins{};
    for (const Bins& table : tables) {  // the serial part: T x 256 additions
        for (int b = 0; b < kBins; ++b) {
            bins[b] += table[b];
        }
    }
    return bins;
}

using Method = Bins (*)(const std::vector<unsigned char>&, unsigned, bool);

double medianMs(const std::vector<unsigned char>& in, Method m, unsigned T, bool interleaved)
{
    std::vector<double> ms;
    for (int run = 0; run < 5; ++run) {
        const auto t0 = std::chrono::steady_clock::now();
        const Bins b = m(in, T, interleaved);
        const auto t1 = std::chrono::steady_clock::now();
        if (b[0] == 0) {
            std::printf("unexpected empty bin 0\n");  // uses the result so it cannot be skipped
        }
        ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(ms.begin(), ms.end());
    return ms[2];
}

void correctness()
{
    const std::size_t n = std::size_t{1} << 20;
    for (bool skewed : {false, true}) {
        const std::vector<unsigned char> in = makeInput(n, skewed);
        const Bins ref = histSerial(in);
        std::printf("%s input, %zu bytes, bin 0 = %lu, bin 255 = %lu\n", skewed ? "skewed" : "uniform", n,
                    ref[0], ref[255]);
        for (unsigned T : {1u, 4u, 8u}) {
            for (bool inter : {false, true}) {
                const bool a = histSharedAtomic(in, T, inter) == ref;
                const bool p = histPrivateMerge(in, T, inter) == ref;
                std::printf("  %u threads, %-11s sharedAtomic %s, privateMerge %s\n", T,
                            inter ? "interleaved" : "chunked", a ? "matches" : "WRONG", p ? "matches" : "WRONG");
            }
        }
    }
}

void timing()
{
    pthread_attr_t attr;
    std::size_t stack = 0;
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stack);
    pthread_attr_destroy(&attr);
    std::printf("hardware_concurrency() = %u; default stack reserved per new thread = %zu KiB\n",
                std::thread::hardware_concurrency(), stack / 1024);

    const int starts = 1000;
    const auto s0 = std::chrono::steady_clock::now();
    for (int i = 0; i < starts; ++i) {
        std::thread t([] {});
        t.join();
    }
    const auto s1 = std::chrono::steady_clock::now();
    std::printf("start + join of one empty std::thread, mean of %d: %.1f microseconds\n", starts,
                std::chrono::duration<double, std::micro>(s1 - s0).count() / starts);

    const std::size_t n = std::size_t{1} << 24;
    for (bool skewed : {false, true}) {
        const std::vector<unsigned char> in = makeInput(n, skewed);
        const double base = medianMs(in, [](const std::vector<unsigned char>& v, unsigned, bool) {
            return histSerial(v);
        }, 1, false);
        std::printf("%s input, %zu bytes: serial %.1f ms\n", skewed ? "skewed (90 % zeros)" : "uniform", n, base);
        const char* names[] = {"sharedAtomic", "privateMerge"};
        const Method methods[] = {histSharedAtomic, histPrivateMerge};
        for (int m = 0; m < 2; ++m) {
            for (bool inter : {false, true}) {
                std::printf("  %-12s %-11s", names[m], inter ? "interleaved" : "chunked");
                for (unsigned T : {1u, 2u, 4u, 8u}) {
                    const double ms = medianMs(in, methods[m], T, inter);
                    std::printf("  T=%u %7.1f ms (x%.2f)", T, ms, base / ms);
                }
                std::printf("\n");
            }
        }
    }

    rusage before{};
    rusage after{};
    getrusage(RUSAGE_SELF, &before);
    const std::vector<unsigned char> in = makeInput(n, false);
    for (int run = 0; run < 5; ++run) {
        histPrivateMerge(in, 8, false);
    }
    getrusage(RUSAGE_SELF, &after);
    std::printf("5 runs of privateMerge with 8 threads on %u CPUs: %ld voluntary and %ld involuntary "
                "context switches (getrusage)\n", std::thread::hardware_concurrency(),
                after.ru_nvcsw - before.ru_nvcsw, after.ru_nivcsw - before.ru_nivcsw);
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "time") {
        timing();
    } else {
        correctness();
    }
    return 0;
}
