// cluster.cpp - BR-10 Listing 3: the same job on the laptop and on three nodes.
// Usage: ./cluster [end of range, default 300000] [timed runs, default 11]
#include "bench.hpp"
#include "cluster.hpp"
#include <cstdlib>

int main(int argc, char** argv)
{
    std::uint64_t const end = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 300000;
    int const reps = argc > 2 ? std::atoi(argv[2]) : 11;
    job::Part const answer = job::countPrimes(0, end);  // the laptop's answer is the oracle

    // The laptop: one process, the F2-51 harness.
    auto const laptop = bench::run([&] { bench::keep(job::countPrimes(0, end).sum); }, 3, reps);

    // The cluster: start three node processes once (like a launcher), then time the job.
    mini::Config cfg;
    cfg.jobEnd = end;
    auto const launchStart = std::chrono::steady_clock::now();
    mini::Cluster c(cfg);
    double const launchMs = std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - launchStart).count();
    c.setLog(false);
    bool same = true;
    mini::JobResult last;
    auto const cluster = bench::run([&] {
        last = c.runJob();
        same = same && last.finished && last.total.count == answer.count &&
               last.total.sum == answer.sum;
    }, 3, reps);

    std::printf("\n--- the job: primes below %llu, split into %d tasks\n",
                static_cast<unsigned long long>(end), cfg.tasks);
    std::printf("laptop answer   : count %llu, sum %llu\n",
                static_cast<unsigned long long>(answer.count),
                static_cast<unsigned long long>(answer.sum));
    std::printf("cluster answers : %s in all %d timed runs\n", same ? "identical" : "DIFFERENT",
                reps);
    std::printf("launch of 3 nodes (fork, listen, connect): %.2f ms, paid once\n", launchMs);
    std::printf("median of %d runs, ms: laptop %.2f   cluster %.2f   speedup %.2f\n", reps,
                laptop.median * 1e3, cluster.median * 1e3, laptop.median / cluster.median);
    std::printf("spread (max-min)/median: laptop %.0f %%   cluster %.0f %%\n",
                100.0 * (laptop.maximum - laptop.minimum) / laptop.median,
                100.0 * (cluster.maximum - cluster.minimum) / cluster.median);
    std::printf("last cluster run, per node (busy time on the node's own clock):\n");
    for (std::size_t i = 0; i < last.perNode.size(); ++i) {
        std::printf("  n%zu: %2d tasks, busy %6.1f ms\n", i + 1, last.perNode[i].tasks,
                    last.perNode[i].busyMs);
    }
    std::printf("\n--- one more run with the controller's structured log switched on\n");
    c.setLog(true);
    mini::JobResult const shown = c.runJob();
    same = same && shown.total.sum == answer.sum;
    return same ? 0 : 1;
}
