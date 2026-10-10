// litmus.cc - BR-06: two litmus tests, each run with weak and with strong orderings.
//   SB (store buffering):  T0: x=1; r0=y        T1: y=1; r1=x        watched: r0=0 and r1=0
//   MP (message passing):  T0: data=1; flag=1   T1: r0=flag; r1=data  watched: r0=1 and r1=0
// The watched outcome is impossible if the two threads' steps simply interleave
// (sequential consistency). Counting how often it appears shows what the CPU (or the
// emulator) really does. Round i uses its own fresh variables, and the two threads
// stream through the rounds together, so that their steps overlap in time.
// Built unchanged for x86-64, AArch64 and RISC-V by run.sh.
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#ifndef ARCH_NAME
#define ARCH_NAME "unknown"
#endif

namespace {

// Round i uses a[i], b[i] and writes r0[i], r1[i]. The four arrays are separate, so the two
// variables of one round sit in different cache lines (as two kernel variables usually do).
struct Rounds {
    explicit Rounds(long n) : a(static_cast<size_t>(n)), b(static_cast<size_t>(n)), r0(a.size()), r1(a.size()) {}
    std::vector<std::atomic<int>> a;   // SB: x    MP: data
    std::vector<std::atomic<int>> b;   // SB: y    MP: flag
    std::vector<int> r0;
    std::vector<int> r1;
};

// Starts both thread bodies at (nearly) the same moment, then joins them.
template <typename F0, typename F1>
void together(F0 f0, F1 f1)
{
    std::atomic<int> ready{0};
    std::thread t1([&] {
        ready.fetch_add(1);
        while (ready.load() < 2) {
        }
        f1();
    });
    ready.fetch_add(1);
    while (ready.load() < 2) {
    }
    f0();
    t1.join();
}

// The orders are template arguments on purpose: a memory order that is not a compile-time
// constant is treated by GCC as seq_cst, which would hide every reordering.
template <std::memory_order st, std::memory_order ld>
long sb(long n)
{
    Rounds r(n);
    together([&] { for (size_t i = 0; i < r.a.size(); ++i) { r.a[i].store(1, st); r.r0[i] = r.b[i].load(ld); } },
             [&] { for (size_t i = 0; i < r.a.size(); ++i) { r.b[i].store(1, st); r.r1[i] = r.a[i].load(ld); } });
    long hits = 0;
    for (size_t i = 0; i < r.a.size(); ++i) {
        hits += (r.r0[i] == 0 && r.r1[i] == 0) ? 1 : 0;
    }
    return hits;
}

template <std::memory_order st, std::memory_order ld>
long mp(long n)
{
    Rounds r(n);
    together([&] { for (size_t i = 0; i < r.a.size(); ++i) { r.a[i].store(1, std::memory_order_relaxed); r.b[i].store(1, st); } },
             [&] { for (size_t i = 0; i < r.a.size(); ++i) { r.r0[i] = r.b[i].load(ld); r.r1[i] = r.a[i].load(std::memory_order_relaxed); } });
    long hits = 0;
    for (size_t i = 0; i < r.a.size(); ++i) {
        hits += (r.r0[i] == 1 && r.r1[i] == 0) ? 1 : 0;
    }
    return hits;
}

} // namespace

int main(int argc, char** argv)
{
    const long n = argc > 1 ? std::atol(argv[1]) : 1000000;
    const int runs = argc > 2 ? std::atoi(argv[2]) : 5;
    std::printf("litmus tests on %s: %d runs of %ld rounds; watched outcome counts per run\n", ARCH_NAME, runs, n);
    const char* names[4] = {"SB relaxed          ", "SB seq_cst          ", "MP relaxed          ",
                            "MP release/acquire  "};
    for (int t = 0; t < 4; ++t) {
        std::printf("  %s", names[t]);
        for (int i = 0; i < runs; ++i) {
            long h = 0;
            switch (t) {
            case 0: h = sb<std::memory_order_relaxed, std::memory_order_relaxed>(n); break;
            case 1: h = sb<std::memory_order_seq_cst, std::memory_order_seq_cst>(n); break;
            case 2: h = mp<std::memory_order_relaxed, std::memory_order_relaxed>(n); break;
            default: h = mp<std::memory_order_release, std::memory_order_acquire>(n); break;
            }
            std::printf(" %7ld", h);
        }
        std::printf("\n");
    }
    return 0;
}
