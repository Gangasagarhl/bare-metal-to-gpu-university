// suite.cpp - MP2 milestone 1 starter: run every scenario of the suite file (stdin)
// with the correct Raft library and print one row per scenario. Exit 0 only if
// every run passes all four checks: safety, linearizability, liveness, determinism.
#include <cstdio>
#include <iostream>

#include "harness.h"

int main()
{
    const std::vector<mp2::Scenario> suite = mp2::parseScenarios(std::cin);
    std::printf("%-19s %5s %4s %9s %6s %6s %8s %7s %9s\n", "scenario", "runs", "fail",
                "answered", "p50", "p99", "recovery", "leaders", "elections");
    int runs = 0;
    int failed = 0;
    for (const mp2::Scenario& s : suite) {
        int n = 0;
        int bad = 0;
        int started = 0;
        int answered = 0;
        int leaders = 0;
        int elections = 0;
        mp2::Time p50 = 0;
        mp2::Time p99 = 0;
        mp2::Time recovery = 0;
        std::vector<mp2::Result> failures;
        for (std::uint64_t seed = s.firstSeed; seed <= s.lastSeed; ++seed) {
            const mp2::Result r = mp2::run(s, seed);
            ++n;
            started += r.started;
            answered += r.answered;
            leaders += r.leaders;
            elections += r.elections;
            p50 = std::max(p50, r.p50);  // worst run of the scenario
            p99 = std::max(p99, r.p99);
            recovery = std::max(recovery, r.recovery);
            if (!r.pass()) {
                ++bad;
                failures.push_back(r);
            }
        }
        runs += n;
        failed += bad;
        std::printf("%-19s %5d %4d %4d/%-4d %4lld ms %3lld ms %5lld ms %7d %9d\n",
                    s.name.c_str(), n, bad, answered, started, static_cast<long long>(p50),
                    static_cast<long long>(p99), static_cast<long long>(recovery), leaders,
                    elections);
        for (const mp2::Result& r : failures) {
            std::printf("    FAIL seed %llu: %s (replay: scenario %s, seed %llu)\n",
                        static_cast<unsigned long long>(r.seed), r.why().c_str(),
                        r.scenario.c_str(), static_cast<unsigned long long>(r.seed));
        }
    }
    std::printf("%d runs, %d failed; every run was executed twice and compared (determinism)\n",
                runs, failed);
    std::printf("times are simulated milliseconds, not measurements of any machine\n");
    return failed == 0 ? 0 : 1;
}
