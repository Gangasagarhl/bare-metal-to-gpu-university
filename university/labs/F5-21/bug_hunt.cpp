// Each deliberate bug of the forensic labs under random fault injection:
// how many seeds expose it, and what does the checker see first?
#include <cstdio>

#include "fuzz.h"

static void hunt(const char* name, raft::Faults f, bool noop, Nemesis nemesis, int seeds)
{
    int caught = 0;
    std::uint64_t first = 0;
    std::string what;
    for (std::uint64_t seed = 1; seed <= static_cast<std::uint64_t>(seeds); ++seed) {
        const RunResult r = fuzzOne(seed, f, noop, nemesis);
        if (!r.violations.empty() || !r.linearizable) {
            if (caught++ == 0) {
                first = seed;
                what = r.violations.empty() ? "client history not linearizable"
                                            : r.violations.front();
            }
        }
    }
    std::printf("%-50s caught in %2d of %d seeds\n", name, caught, seeds);
    if (caught > 0) {
        std::printf("    first: seed %llu: %s\n", static_cast<unsigned long long>(first),
                    what.c_str());
    }
}

int main()
{
    raft::Faults f;
    f.lazyPersist = true;
    hunt("lazyPersist, random nemesis", f, true, Nemesis::Random, 100);
    f = {};
    f.skipPrevLogCheck = true;
    hunt("skipPrevLogCheck, random nemesis", f, true, Nemesis::Random, 100);
    f = {};
    f.readFromLocalState = true;
    hunt("readFromLocalState, random nemesis", f, true, Nemesis::Random, 100);
    f = {};
    f.commitOldTermByCount = true;
    hunt("commitOldTermByCount, random nemesis, no no-op", f, false, Nemesis::Random, 100);
    hunt("commitOldTermByCount, leader hunter, no no-op", f, false, Nemesis::LeaderHunter, 150);
    hunt("commitOldTermByCount, leader hunter, with no-op", f, true, Nemesis::LeaderHunter, 150);
    hunt("correct Raft, leader hunter", raft::Faults{}, true, Nemesis::LeaderHunter, 150);
    return 0;
}
