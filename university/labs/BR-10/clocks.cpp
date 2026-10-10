// clocks.cpp - BR-10 Listing 8: the trap "assuming clocks agree".
// All three node processes share this machine's clock, so the error is injected:
// n2's clock reads 400 ms ahead, n3's 300 ms behind. Timestamps from different clocks
// are then compared naively, and after a round-trip estimate of each offset.
#include "cluster.hpp"
#include <algorithm>

int main()
{
    long const injected[3] = {0, 400, -300};
    mini::Config cfg;
    cfg.log = false;
    for (std::size_t i = 0; i < 3; ++i) {
        cfg.nodes[i].clockOffsetMs = injected[i];
    }
    mini::Cluster c(cfg);

    std::printf("1. offset of each node's clock, estimated from 5 round trips (best kept)\n");
    long long est[3], half[3];
    for (std::size_t i = 0; i < 3; ++i) {
        est[i] = c.estimateOffsetUs(i, 5, half[i]);
        std::printf("   n%zu: injected %+5ld ms, estimated %+9.3f ms, error bound +/- %.3f ms\n",
                    i + 1, injected[i], static_cast<double>(est[i]) / 1000.0,
                    static_cast<double>(half[i]) / 1000.0);
    }

    mini::JobResult const r = c.runJob();
    std::vector<mini::Cluster::Event> ev = c.events;
    std::sort(ev.begin(), ev.end(), [](auto const& a, auto const& b) { return a.task < b.task; });
    long long const t0 = ev.front().assignedWall;
    auto ms = [](long long us) { return static_cast<double>(us) / 1000.0; };

    std::printf("\n2. per task: queue delay = node start - controller send (two clocks!)\n");
    std::printf("   task node  delay naive  delay corrected  duration (one clock)\n");
    int impossibleNaive = 0, impossibleFixed = 0;
    for (auto const& e : ev) {
        long long const naive = e.nodeStart - e.assignedWall;
        long long const fixed = e.nodeStart - est[e.node] - e.assignedWall;
        if (e.nodeEnd < e.assignedWall || e.nodeEnd > e.receivedWall) {
            ++impossibleNaive;  // finished before it was sent, or after its answer arrived
        }
        long long const endFixed = e.nodeEnd - est[e.node];
        if (endFixed < e.assignedWall - half[e.node] || endFixed > e.receivedWall + half[e.node]) {
            ++impossibleFixed;  // outside even when the estimate's error bound is allowed
        }
        if (e.task < 6) {
            std::printf("   %4d  n%zu  %+10.3f ms  %+12.3f ms  %10.3f ms\n", e.task, e.node + 1,
                        ms(naive), ms(fixed), ms(e.nodeEnd - e.nodeStart));
        }
    }
    std::printf("   (tasks 6-11 omitted; the job's answer: count %llu, sum %llu)\n",
                static_cast<unsigned long long>(r.total.count),
                static_cast<unsigned long long>(r.total.sum));

    std::printf("\n3. a merged timeline sorted by raw timestamps (ms after the first send)\n");
    struct Line { long long when; std::string what; };
    std::vector<Line> lines;
    for (auto const& e : ev) {
        if (e.task < 3) {
            std::string const n = "n" + std::to_string(e.node + 1);
            lines.push_back({e.assignedWall, "controller sends task " + std::to_string(e.task) +
                                                 " to " + n});
            lines.push_back({e.nodeEnd, n + " finishes task " + std::to_string(e.task)});
            lines.push_back({e.receivedWall, "controller receives task " +
                                                 std::to_string(e.task)});
        }
    }
    std::stable_sort(lines.begin(), lines.end(),
                     [](Line const& a, Line const& b) { return a.when < b.when; });
    for (auto const& l : lines) {
        std::printf("   %+10.3f  %s\n", ms(l.when - t0), l.what.c_str());
    }
    std::printf("\n4. tasks whose finish lies outside [sent, received]: raw %d of %zu,"
                " corrected %d of %zu\n", impossibleNaive, ev.size(), impossibleFixed, ev.size());
    return r.finished && r.total.count == 25997 ? 0 : 1;
}
