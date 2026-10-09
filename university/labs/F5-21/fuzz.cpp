// The correct implementation under 100 random fault schedules.
#include <cstdio>

#include "fuzz.h"

int main()
{
    int failed = 0;
    long operations = 0;
    long answered = 0;
    long leaders = 0;
    for (std::uint64_t seed = 1; seed <= 100; ++seed) {
        const RunResult r = fuzzOne(seed, raft::Faults{});
        operations += r.operations;
        answered += r.answered;
        leaders += r.leaders;
        if (!r.violations.empty() || !r.linearizable) {
            ++failed;
            std::printf("seed %llu FAILED: %s\n", static_cast<unsigned long long>(seed),
                        r.violations.empty() ? "history not linearizable"
                                             : r.violations.front().c_str());
        }
    }
    std::printf("100 seeds: %d failed; %ld client operations, %ld answered; "
                "%ld elected leaders in total\n", failed, operations, answered, leaders);
    return failed == 0 ? 0 : 1;
}
