// forensic.cpp - generates the evidence pack of BR-10's forensic lab.
// What was injected is written in the chapter's answer key, not here in the output.
#include "bench.hpp"
#include "cluster.hpp"
#include <algorithm>

int main()
{
    mini::Config cfg;
    cfg.assign = mini::Assign::fixed;   // task t always goes to node t % 3
    cfg.nodes[1].queueMs = 25;
    cfg.nodes[2].clockOffsetMs = -300;
    auto const laptop = bench::run([&] { bench::keep(job::countPrimes(0, cfg.jobEnd).sum); }, 3, 21);
    std::printf("== evidence 1: laptop, same job, 21 runs: median %.1f ms\n\n",
                laptop.median * 1e3);
    std::printf("== evidence 2: controller log of the cluster run\n");
    mini::Cluster c(cfg);
    mini::JobResult const r = c.runJob();
    std::vector<mini::Cluster::Event> ev = c.events;
    long long const t0 = std::min_element(ev.begin(), ev.end(), [](auto const& a, auto const& b) {
                             return a.assignedWall < b.assignedWall;
                         })->assignedWall;
    std::printf("\n== evidence 3: the nodes' own logs (each node's clock, ms after the "
                "controller's first send)\n");
    for (std::size_t n = 0; n < 3; ++n) {
        for (auto const& e : ev) {
            if (e.node == n) {
                std::printf("   n%zu clock=%+9.1f event=start task=%d\n   n%zu clock=%+9.1f "
                            "event=end task=%d\n", n + 1,
                            static_cast<double>(e.nodeStart - t0) / 1000.0, e.task, n + 1,
                            static_cast<double>(e.nodeEnd - t0) / 1000.0, e.task);
            }
        }
    }
    std::printf("\n== evidence 4: job summary: %s in %.1f ms, count %llu\n",
                r.finished ? "finished" : "not finished", r.seconds * 1e3,
                static_cast<unsigned long long>(r.total.count));
    return 0;
}
