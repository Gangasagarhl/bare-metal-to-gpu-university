// replay.cpp - MP2 milestone 1 starter: replay one run of the suite exactly, with
// the servers' own trace lines printed for a time window. Because the simulator is
// deterministic, a failing (scenario, seed, variant) from mutants.out or suite.out
// is a complete bug report: this program reproduces it byte for byte.
#include <cstdio>
#include <fstream>
#include <string>

#include "harness.h"

int main(int argc, char** argv)
{
    // Defaults: the first catch of readFromLocalState in mutants.out.
    const std::string name = argc > 1 ? argv[1] : "leader-isolated";
    const std::uint64_t seed = argc > 2 ? std::stoull(argv[2]) : 12;
    const mp2::Time from = argc > 3 ? std::stoll(argv[3]) : 495;
    const mp2::Time to = argc > 4 ? std::stoll(argv[4]) : 700;

    std::ifstream file("suite.in");
    for (const mp2::Scenario& s : mp2::parseScenarios(file)) {
        if (s.name != name) {
            continue;
        }
        mp2::Variant v;
        v.name = "readFromLocalState";
        v.faults.readFromLocalState = true;
        std::printf("replay: scenario %s, seed %llu, variant %s, trace %lld-%lld ms\n",
                    name.c_str(), static_cast<unsigned long long>(seed), v.name.c_str(),
                    static_cast<long long>(from), static_cast<long long>(to));
        const mp2::Result r = mp2::runOnce(s, seed, v, from, to);
        std::printf("verdict: %s; %d of %d client operations answered; fingerprint %016llx\n",
                    r.pass() ? "pass" : r.why().c_str(), r.answered, r.started,
                    static_cast<unsigned long long>(r.fingerprint));
        std::printf("client operations called while the fault was active (500-1500 ms):\n");
        std::vector<raft::HistoryOp> window;
        for (const raft::HistoryOp& h : r.history) {
            if (h.invoke >= 500 && h.invoke <= 1500) {
                window.push_back(h);
            }
        }
        lin::printHistory(window);
        const mp2::Result again = mp2::runOnce(s, seed, v);
        std::printf("second run:  fingerprint %016llx (%s)\n",
                    static_cast<unsigned long long>(again.fingerprint),
                    again.fingerprint == r.fingerprint ? "identical" : "DIFFERENT");
        return 0;
    }
    std::printf("no scenario named %s in suite.in\n", name.c_str());
    return 2;
}
