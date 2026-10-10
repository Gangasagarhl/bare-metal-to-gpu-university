// mutants.cpp - MP2 milestone 1 starter: how strong is the suite? Switch on each
// planted bug of the DS302 library (raft::Faults), or a changed option, and count,
// per scenario of suite.in, the runs in which the four checks catch it. Rows that
// catch nothing are printed as SURVIVED: an honest gap in the suite, not a pass.
#include <cstdio>
#include <fstream>

#include "harness.h"

int main()
{
    std::ifstream file("suite.in");
    const std::vector<mp2::Scenario> suite = mp2::parseScenarios(file);

    std::vector<mp2::Variant> variants(7);
    variants[0].name = "correct library";
    variants[1].name = "lazyPersist";
    variants[1].faults.lazyPersist = true;
    variants[2].name = "skipPrevLogCheck";
    variants[2].faults.skipPrevLogCheck = true;
    variants[3].name = "readFromLocalState";
    variants[3].faults.readFromLocalState = true;
    variants[4].name = "commitOldTermByCount";
    variants[4].faults.commitOldTermByCount = true;
    variants[5].name = "commitOldTermByCount, no no-op";
    variants[5].faults.commitOldTermByCount = true;
    variants[5].options.noopOnElection = false;
    variants[6].name = "heartbeat 200 ms (an option)";
    variants[6].options.heartbeat = 200;  // a configuration mistake, not a code bug

    std::printf("%-31s", "variant \\ scenario");
    for (std::size_t i = 0; i < suite.size(); ++i) {
        std::printf(" %5c", static_cast<char>('A' + i));
    }
    std::printf("  verdict\n");
    for (const mp2::Variant& v : variants) {
        std::printf("%-31s", v.name.c_str());
        int caught = 0;
        std::string first;
        for (const mp2::Scenario& s : suite) {
            int k = 0;
            int n = 0;
            for (std::uint64_t seed = s.firstSeed; seed <= s.lastSeed; ++seed, ++n) {
                const mp2::Result r = mp2::runOnce(s, seed, v);
                if (!r.pass()) {
                    ++k;
                    if (first.empty()) {
                        first = s.name + " seed " + std::to_string(seed) + ": " + r.why();
                    }
                }
            }
            caught += k;
            std::printf(" %2d/%-2d", k, n);
        }
        const bool isCorrect = v.name == "correct library";
        std::printf("  %s\n", isCorrect ? (caught == 0 ? "no false alarm" : "FALSE ALARM")
                                        : (caught > 0 ? "killed" : "SURVIVED"));
        if (!first.empty()) {
            std::printf("    first catch: %s\n", first.c_str());
        }
    }
    std::printf("scenarios:");
    for (std::size_t i = 0; i < suite.size(); ++i) {
        std::printf(" %c=%s", static_cast<char>('A' + i), suite[i].name.c_str());
    }
    std::printf("\n");
    return 0;
}
