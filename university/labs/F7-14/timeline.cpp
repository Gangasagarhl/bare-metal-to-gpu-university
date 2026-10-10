// F7-14 Listing 3: what a systems profiler's timeline shows, in a model. A host loop launches
// small kernels; version A waits for each kernel before preparing the next (a sync per launch),
// version B queues all launches and waits once. The model prints a kernel trace like a
// tracer would, and the analysis a timeline view makes visible: GPU busy share and gaps.
// All durations are INVENTED model units (us), not measurements.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

struct Event {
    std::string name;
    double start;
    double end;
};

// launchCost: host time to issue one launch; work: kernel duration; prep: host work per step;
// syncEach: wait for the kernel before the next step's host work.
std::vector<Event> runModel(int steps, double launchCost, double work, double prep, bool syncEach)
{
    std::vector<Event> gpu;
    double host = 0.0;      // host clock
    double gpuFree = 0.0;   // when the GPU finishes its queue
    for (int s = 0; s < steps; ++s) {
        host += prep;                                       // host prepares the step
        host += launchCost;                                 // the launch call returns
        const double start = std::max(host, gpuFree);       // kernel starts when queued and GPU free
        const double end = start + work;
        gpu.push_back({"step" + std::to_string(s), start, end});
        gpuFree = end;
        if (syncEach) {
            host = std::max(host, end);                     // e.g. a synchronize after every launch
        }
    }
    return gpu;
}

void analyse(const char* title, const std::vector<Event>& gpu, bool printTrace)
{
    double busy = 0.0, longestGap = 0.0;
    for (std::size_t k = 0; k < gpu.size(); ++k) {
        busy += gpu[k].end - gpu[k].start;
        if (k > 0) {
            longestGap = std::max(longestGap, gpu[k].start - gpu[k - 1].end);
        }
    }
    const double span = gpu.back().end - gpu.front().start;
    std::printf("%s\n", title);
    if (printTrace) {
        std::printf("  kernel   start_us   end_us   gap_before_us\n");
        for (std::size_t k = 0; k < 5 && k < gpu.size(); ++k) {
            std::printf("  %-7s %9.1f %8.1f %15.1f\n", gpu[k].name.c_str(), gpu[k].start, gpu[k].end,
                        k == 0 ? 0.0 : gpu[k].start - gpu[k - 1].end);
        }
        std::printf("  ... (%zu kernels)\n", gpu.size());
    }
    std::printf("  span %.1f us, GPU busy %.1f us = %.1f %%, longest gap %.1f us\n", span, busy,
                100.0 * busy / span, longestGap);
}

int main()
{
    const int steps = 100;
    const double launch = 5.0, work = 20.0, prep = 15.0;
    std::printf("model: %d steps, launch call %.0f us, kernel %.0f us, host preparation %.0f us per step\n",
                steps, launch, work, prep);
    analyse("A: synchronize after every launch", runModel(steps, launch, work, prep, true), true);
    analyse("B: queue all launches, synchronize once", runModel(steps, launch, work, prep, false), true);
    analyse("C: as B, but tiny kernels of 3 us and no host preparation (launch-bound)",
            runModel(steps, launch, 3.0, 0.0, false), false);
    return 0;
}
