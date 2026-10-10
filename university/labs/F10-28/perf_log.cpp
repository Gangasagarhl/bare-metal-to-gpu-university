// perf_log.cpp - F10-28 forensic evidence generator. The 400 Hz loop of Listing 1 with one
// extra task, "object_avoid", which the developer declared with a 300 us budget. Its real
// cost is small while the feature is switched off and large after the pilot switches it on
// at t = 8 s. The program prints what a flight stack's once-per-second performance message
// could contain. SYNTHETIC evidence: our simulator, not a real vehicle (see the answer key).
#include <cstdio>
#include <vector>

#include "aploop.hpp"

int main()
{
    std::vector<aploop::Task> table = {
        {"rc_input", 400, 100, 40},     {"object_avoid", 400, 300, 100},
        {"gps", 50, 200, 120},          {"navigation", 100, 300, 180},
        {"compass", 100, 100, 60},      {"baro", 50, 150, 90},
        {"telemetry", 50, 500, 300},    {"logging", 100, 400, 250},
        {"battery", 10, 120, 70},       {"arming", 1, 50, 30},
    };
    constexpr int loopHz = 400;
    constexpr int periodUs = 1000000 / loopHz;
    std::vector<int> lastRuns(table.size(), 0);
    int lastOver = 0;
    int overrunsThisSecond = 0;
    int worstThisSecond = 0;

    std::printf("PERF  t(s)  loops  overruns  worst_us  rc_in  telem  log  batt  avoid_over\n");
    aploop::Result total = aploop::simulate(
        table, loopHz, 1100, 17, [&](int loop, std::vector<aploop::Task>& t) {
            if (loop == 8 * loopHz) {
                t[1].costUs = 900;            // pilot switches the feature on at t = 8 s
            }
            if (loop > 0 && loop % loopHz == 0) {
                const int sec = loop / loopHz;
                std::printf("PERF  %4d  %5d  %8d  %8d  %5d  %5d  %3d  %4d  %10d\n", sec, loopHz,
                            overrunsThisSecond, worstThisSecond, t[0].runs - lastRuns[0],
                            t[6].runs - lastRuns[6], t[7].runs - lastRuns[7],
                            t[8].runs - lastRuns[8], t[1].overBudget - lastOver);
                for (std::size_t i = 0; i < t.size(); ++i) {
                    lastRuns[i] = t[i].runs;
                }
                lastOver = t[1].overBudget;
                overrunsThisSecond = 0;
                worstThisSecond = 0;
            }
        },
        [&](int usedUs) {
            if (usedUs > periodUs) { ++overrunsThisSecond; }
            if (usedUs > worstThisSecond) { worstThisSecond = usedUs; }
        });
    std::printf("total: loops %d, loop overruns %d, worst loop %d us (period %d us)\n",
                total.loops, total.loopOverruns, total.worstLoopUs, periodUs);
    return 0;
}
