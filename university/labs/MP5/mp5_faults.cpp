// MP5 starter, Listing 3: fault handling in the ring (seed of milestone 4, curriculum F8).
// One rank stops silently in the middle of an all-reduce. Required behaviour:
//   (a) no rank hangs: every survivor returns an error (timeout or aborted) within a bound;
//   (b) the aborted buffers are not trusted: re-running on them gives wrong sums;
//   (c) recovery = a new ring plus the inputs saved before the call (the checkpoint idea).
// Which survivor times out first depends on thread timing, so only counts are printed.
#include "mp5_check.h"

#include <cstdio>

namespace {

using Clock = std::chrono::steady_clock;
constexpr std::size_t kN = 4;
constexpr std::size_t kCount = 100000;
constexpr std::size_t kChunk = 1024;
constexpr auto kTimeout = std::chrono::milliseconds(300);
constexpr double kBoundSec = 3.0;   // generous: sanitizers and a shared machine

struct Outcome
{
    std::vector<mp5::Status> status;
    double seconds;
};

Outcome runRing(std::vector<std::vector<float>>& buf, int stopRank, int stopStep)
{
    mp5::ThreadRing<float> ring(kN, kTimeout);
    std::vector<mp5::ThreadRing<float>::ThreadLink> links;
    links.reserve(kN);
    for (std::size_t r = 0; r < kN; ++r) {
        links.emplace_back(ring, r);
    }
    Outcome o{std::vector<mp5::Status>(kN, mp5::Status::ok), 0.0};
    const auto t0 = Clock::now();
    std::vector<std::thread> threads;
    for (std::size_t r = 0; r < kN; ++r) {
        threads.emplace_back([&, r] {
            const int stop = static_cast<int>(r) == stopRank ? stopStep : -1;
            o.status[r] = mp5::ringAllReduce<float>(links[r], r, kN, std::span<float>(buf[r]), kChunk, stop);
        });
    }
    for (std::thread& t : threads) {
        t.join();
    }
    o.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
    return o;
}

std::vector<std::vector<float>> inputs()
{
    std::vector<std::vector<float>> buf(kN, std::vector<float>(kCount));
    for (std::size_t r = 0; r < kN; ++r) {
        for (std::size_t i = 0; i < kCount; ++i) {
            buf[r][i] = mp5::inputValue<float>(r, i);
        }
    }
    return buf;
}

// elements (on any rank) outside the error bound of the double reference
std::size_t wrongElements(const std::vector<std::vector<float>>& buf)
{
    std::size_t bad = 0;
    for (std::size_t i = 0; i < kCount; ++i) {
        double ref = 0.0;
        double mag = 0.0;
        for (std::size_t r = 0; r < kN; ++r) {
            const double x = static_cast<double>(mp5::inputValue<float>(r, i));
            ref += x;
            mag += std::fabs(x);
        }
        const double bound = static_cast<double>(kN - 1) * std::ldexp(1.0, -24) * mag;
        for (std::size_t r = 0; r < kN; ++r) {
            bad += std::fabs(static_cast<double>(buf[r][i]) - ref) > bound ? 1 : 0;
        }
    }
    return bad;
}

bool scenario(const char* title, int stopRank, int stopStep)
{
    std::printf("== %s\n", title);
    std::vector<std::vector<float>> buf = inputs();
    const std::vector<std::vector<float>> saved = buf;          // the "checkpoint" of the inputs
    const Outcome o = runRing(buf, stopRank, stopStep);
    std::size_t survivors = 0;
    std::size_t errors = 0;
    for (std::size_t r = 0; r < kN; ++r) {
        if (static_cast<int>(r) == stopRank) {
            std::printf("  rank %zu: %s\n", r, mp5::name(o.status[r]));
            continue;
        }
        ++survivors;
        errors += (o.status[r] == mp5::Status::timeout || o.status[r] == mp5::Status::aborted) ? 1 : 0;
    }
    const bool inTime = o.seconds < kBoundSec;
    if (stopRank < 0) {
        const bool ok = errors == 0 && wrongElements(buf) == 0;
        std::printf("  all %zu ranks ok and every value within its bound: %s\n", kN, ok ? "yes" : "NO");
        return ok;
    }
    std::printf("  survivors that returned an error (timeout or aborted): %zu of %zu\n", errors, survivors);
    std::printf("  all ranks returned within %.1f s (timeout per wait %lld ms): %s\n", kBoundSec,
                static_cast<long long>(kTimeout.count()), inTime ? "yes" : "NO");
    // (b) re-running on the half-reduced buffers
    std::vector<std::vector<float>> reused = buf;
    runRing(reused, -1, -1);
    const std::size_t badReused = wrongElements(reused);
    std::printf("  re-run on the aborted buffers: values outside the bound: %s\n", badReused > 0 ? "yes (as expected)" : "none");
    // (c) recovery from the saved inputs on a new ring
    std::vector<std::vector<float>> fresh = saved;
    const Outcome o2 = runRing(fresh, -1, -1);
    bool allOk = true;
    for (mp5::Status s : o2.status) {
        allOk = allOk && s == mp5::Status::ok;
    }
    const std::size_t badFresh = wrongElements(fresh);
    std::printf("  new ring on the saved inputs: all ok %s, values outside the bound: %zu\n", allOk ? "yes" : "NO", badFresh);
    return errors == survivors && inTime && badReused > 0 && allOk && badFresh == 0;
}

}  // namespace

int main()
{
    bool ok = true;
    ok = scenario("no fault", -1, -1) && ok;
    ok = scenario("rank 2 stops before global step 1 (reduce-scatter)", 2, 1) && ok;
    ok = scenario("rank 0 stops before global step 4 (all-gather)", 0, 4) && ok;
    std::printf("fault handling (CPU stand-in): %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
