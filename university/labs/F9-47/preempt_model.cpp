// preempt_model.cpp - F9-47 Listing 2: a toy model of ONE processor, to show why the
// preemption model decides the worst wake-up latency of a high-priority task.
// It is a model, not a measurement: every duration below is an assumption chosen for
// teaching (the comments say so). The shapes of the results are the lesson, not the numbers.
//
// A real-time task is released by a timer every 1000 us. Meanwhile the processor runs
// background activity: ordinary preemptible code, kernel sections that disable preemption,
// hard interrupt handlers and deferred interrupt work ("softirq"-style bottom halves).
// Three configurations decide what the woken task must wait for:
//   stock     : kernel sections, hard handlers and bottom halves all run to completion first.
//   rt        : (PREEMPT_RT style) kernel sections are preemptible except a short raw part;
//               handlers run in threads at priority 50; the task has priority 80.
//   rt-prio40 : the same kernel, but the task was given priority 40, below the handler threads.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

enum class Kind { User, KernelSection, HardIrq, BottomHalf };

struct Segment {
    long start;
    long length;
    Kind kind;
    const char* what;
};

std::uint64_t rng = 0x9E3779B97F4A7C15ULL;          // fixed seed: the run repeats exactly
long draw(long lo, long hi)                          // xorshift64*, uniform in [lo, hi]
{
    rng ^= rng >> 12; rng ^= rng << 25; rng ^= rng >> 27;
    const std::uint64_t r = rng * 0x2545F4914F6CDD1DULL;
    return lo + static_cast<long>(r % static_cast<std::uint64_t>(hi - lo + 1));
}

// ASSUMED background load, back to back for `horizon` microseconds.
std::vector<Segment> background(long horizon)
{
    std::vector<Segment> seg;
    long t = 0;
    while (t < horizon) {
        const long r = draw(0, 99);
        Segment s{t, 0, Kind::User, "user code"};
        if (r < 55) {
            s = {t, draw(20, 400), Kind::User, "user code"};
        } else if (r < 85) {
            s = {t, draw(2, 60), Kind::KernelSection, "short kernel section"};
        } else if (r < 93) {
            s = {t, draw(5, 30), Kind::HardIrq, "network handler"};
        } else if (r < 97) {
            s = {t, draw(100, 600), Kind::BottomHalf, "network bottom half"};
        } else if (r < 99) {
            s = {t, draw(300, 1500), Kind::KernelSection, "long kernel section"};
        } else {
            s = {t, draw(200, 900), Kind::HardIrq, "storage handler"};
        }
        seg.push_back(s);
        t += s.length;
    }
    return seg;
}

constexpr long kSwitchCost = 4;     // ASSUMED cost of waking and switching to the task (us)
constexpr long kRawPart = 8;        // ASSUMED non-preemptible part left in an rt kernel (us)
constexpr long kHardPart = 3;       // ASSUMED handler part that stays in hard-irq context (us)

// How long must the woken task wait if it is released at time t inside segment s?
long waitFor(const Segment& s, long t, const std::string& cfg)
{
    const long left = s.start + s.length - t;
    const long into = t - s.start;
    if (cfg == "stock") {
        return s.kind == Kind::User ? 0 : left;            // everything else runs to the end
    }
    // rt and rt-prio40: only short raw parts are non-preemptible
    if (s.kind == Kind::User) { return 0; }
    if (s.kind == Kind::KernelSection) { return std::max(0L, std::min(kRawPart, s.length) - into); }
    // handlers: a short hard part, then a thread at priority 50
    const long hard = std::max(0L, std::min(kHardPart, s.length) - into);
    if (cfg == "rt-prio40") { return left; }               // the thread outranks the task
    return hard;
}

void run(const std::string& cfg, const std::vector<Segment>& seg, long horizon, bool trace)
{
    std::vector<long> lat;
    struct Worst { long latency; long when; const char* what; };
    std::vector<Worst> worst;
    std::size_t i = 0;
    for (long t = 1000; t < horizon; t += 1000) {
        while (seg[i].start + seg[i].length <= t) { ++i; }
        long l = kSwitchCost + waitFor(seg[i], t, cfg);
        // in "stock", a non-preemptible segment may be followed by another one
        for (std::size_t j = i + 1; cfg == "stock" && waitFor(seg[i], t, cfg) > 0 &&
                                    j < seg.size() && seg[j].kind != Kind::User &&
                                    seg[j].kind != Kind::KernelSection; ++j) {
            l += seg[j].length;                            // a handler that was pending
        }
        lat.push_back(l);
        worst.push_back({l, t, seg[i].what});
    }
    std::vector<long> sorted = lat;
    std::sort(sorted.begin(), sorted.end());
    auto pct = [&](double p) {
        return sorted[std::min(sorted.size() - 1,
                               static_cast<std::size_t>(p / 100.0 * static_cast<double>(sorted.size())))];
    };
    std::printf("configuration %-9s  wake-ups %zu  min %ld  median %ld  p99 %ld  p99.9 %ld  max %ld us\n",
                cfg.c_str(), lat.size(), sorted.front(), pct(50), pct(99), pct(99.9), sorted.back());
    const long edges[] = {10, 20, 50, 100, 200, 500, 1000, 2000};
    long lo = 0;
    for (long e : edges) {
        const auto n = std::count_if(lat.begin(), lat.end(), [&](long v) { return v >= lo && v < e; });
        std::printf("  %5ld-%-5ld us %6ld %s\n", lo, e - 1, static_cast<long>(n),
                    std::string(static_cast<std::size_t>((n + 199) / 200), '#').c_str());
        lo = e;
    }
    const auto over = std::count_if(lat.begin(), lat.end(), [&](long v) { return v >= lo; });
    std::printf("  >= %-8ld us %6ld %s\n", lo, static_cast<long>(over),
                std::string(static_cast<std::size_t>((over + 199) / 200), '#').c_str());
    if (trace) {
        std::sort(worst.begin(), worst.end(), [](const Worst& a, const Worst& b) {
            return a.latency > b.latency || (a.latency == b.latency && a.when < b.when);
        });
        std::printf("  the 6 worst wake-ups and what was running at the release:\n");
        for (std::size_t k = 0; k < 6 && k < worst.size(); ++k) {
            std::printf("    t=%8ld us  latency %5ld us  running: %s\n", worst[k].when,
                        worst[k].latency, worst[k].what);
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const long horizon = 10'000'000;                       // 10 s of model time
    const std::vector<Segment> seg = background(horizon + 5000);
    const bool forensic = argc > 1 && std::strcmp(argv[1], "forensic") == 0;
    std::printf("toy model: 1 processor, timer release every 1000 us, 10 s; histogram: "
                "# = up to 200 wake-ups\n");
    if (forensic) {
        run("rt-prio40", seg, horizon, true);
    } else {
        run("stock", seg, horizon, true);
        run("rt", seg, horizon, true);
    }
    return 0;
}
