// aploop.hpp - F10-28: the university's model of a flight stack's main loop with a task
// table. Every loop period the "fast loop" runs first (sensors, rate control, outputs); then
// the scheduler walks the table in order and runs each task that is due AND whose time
// budget fits into what is left of the period. The structure follows the idea that the
// ArduPilot documentation describes for its vehicle code; all names and numbers here are
// our own exercise values (see the chapter's unverified box for the real ones).
#pragma once
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace aploop {

struct Task {
    std::string name;
    int rateHz;        // how often the task wants to run
    int budgetUs;      // the time it declares it needs (its "max time")
    int costUs;        // what it really takes, before jitter
    // results
    int runs = 0;
    int overBudget = 0;    // runs that took longer than the declared budget
    int skipped = 0;       // loops in which it was due but did not fit
    int lastRunLoop = -1000000;
    int longestGapLoops = 0;
};

struct Result {
    int loops = 0;
    int loopOverruns = 0;  // loops that ended after the next loop should have started
    int worstLoopUs = 0;
};

// Deterministic pseudo-random jitter (a linear congruential generator), so runs repeat.
struct Jitter {
    std::uint32_t state = 12345u;
    int next(int spreadUs)
    {
        state = state * 1103515245u + 12345u;
        return static_cast<int>((state >> 16) % static_cast<std::uint32_t>(spreadUs + 1));
    }
};

// onLoop (optional) is called at the start of every loop and onLoopEnd with the time the
// loop used; the forensic lab uses them to change a task's cost at a given time and to
// print statistics once per second.
inline Result simulate(std::vector<Task>& table, int loopHz, int fastLoopUs, int seconds,
                       const std::function<void(int, std::vector<Task>&)>& onLoop = nullptr,
                       const std::function<void(int)>& onLoopEnd = nullptr)
{
    const int periodUs = 1000000 / loopHz;
    Jitter jitter;
    Result r;
    for (int loop = 0; loop < loopHz * seconds; ++loop) {
        if (onLoop) {
            onLoop(loop, table);
        }
        int used = fastLoopUs + jitter.next(50);
        for (Task& t : table) {
            const int interval = loopHz / t.rateHz;          // loops between runs
            const bool due = loop - t.lastRunLoop >= interval;
            if (!due) {
                continue;
            }
            if (used + t.budgetUs > periodUs) {               // does not fit: try next loop
                ++t.skipped;
                continue;
            }
            const int took = t.costUs + jitter.next(t.costUs / 4);
            used += took;
            if (took > t.budgetUs) {
                ++t.overBudget;
            }
            if (t.lastRunLoop >= 0 && loop - t.lastRunLoop > t.longestGapLoops) {
                t.longestGapLoops = loop - t.lastRunLoop;
            }
            t.lastRunLoop = loop;
            ++t.runs;
        }
        if (onLoopEnd) {
            onLoopEnd(used);
        }
        if (used > periodUs) {
            ++r.loopOverruns;
        }
        if (used > r.worstLoopUs) {
            r.worstLoopUs = used;
        }
        ++r.loops;
    }
    return r;
}

inline void report(const std::vector<Task>& table, const Result& r, int loopHz, int seconds)
{
    std::printf("%-12s %5s %7s %6s %6s %8s %8s %10s\n", "task", "Hz", "budget", "want",
                "runs", "skipped", "over", "max gap");
    for (const Task& t : table) {
        const int want = t.rateHz * seconds;
        std::printf("%-12s %5d %5d us %6d %6d %8d %8d %7.1f ms\n", t.name.c_str(), t.rateHz,
                    t.budgetUs, want, t.runs, t.skipped, t.overBudget,
                    t.longestGapLoops * 1000.0 / loopHz);
    }
    std::printf("loops %d, loop overruns %d, worst loop %d us of %d us\n", r.loops,
                r.loopOverruns, r.worstLoopUs, 1000000 / loopHz);
}

}  // namespace aploop
