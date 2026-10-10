// F10-07 forensic evidence "The loop that stutters": the rate-loop timing log that the
// university's simulated flight controller writes. The task set is hidden in the
// answer key; the learner sees only the log. (Synthetic: produced by fcsim.hpp.)
#include "fcsim.hpp"

#include <cstdio>
#include <map>
#include <vector>

int main()
{
    std::vector<Task> tasks{{"rate loop", 1000, 250, 0, 0},
                            {"sensors", 1000, 150, 1, 0},
                            {"estimator", 4000, 600, 2, 0},
                            {"position", 20000, 1500, 3, 0},
                            {"logger", 20000, 2500, 4, 2500}};
    std::vector<StartRecord> trace;
    simulate(tasks, 1000000, &trace);

    std::printf("rate-loop timing log, first 30 ms (release and start in us)\n");
    std::printf("cycle  release   start  late\n");
    int expected = 0;
    for (const auto& r : trace) {
        while (expected < r.release && expected < 30000) {
            std::printf("%5d %8d       -  SKIPPED (no start before next release)\n",
                        expected / 1000, expected);
            expected += 1000;
        }
        if (r.release >= 30000) {
            break;
        }
        std::printf("%5d %8d %7d %5d\n", r.release / 1000, r.release, r.start,
                    r.start - r.release);
        expected = r.release + 1000;
    }
    std::map<int, int> histogram;
    for (const auto& r : trace) {
        ++histogram[(r.start - r.release) / 100 * 100];
    }
    std::printf("\nwhole log (1 s): %zu starts out of %d releases\n", trace.size(), tasks[0].jobs);
    std::printf("start lateness histogram (100 us bins)\n");
    for (const auto& [bin, n] : histogram) {
        std::printf("  %4d..%4d us: %4d\n", bin, bin + 99, n);
    }
    return 0;
}
