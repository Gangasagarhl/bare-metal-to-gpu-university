// Two proposers that keep overtaking each other ("dueling proposers"): a fixed
// retry timeout versus a timeout with a random part; 100 seeds each. Measured:
// when does a proposer first LEARN that a value is chosen (a majority accepted
// its ballot)?
#include "paxos.h"

static void study(const char* label, sim::Time fixed, sim::Time randomExtra, bool two = true)
{
    int decided = 0;
    long ballots = 0;
    sim::Time slowest = 0;
    for (std::uint64_t seed = 1; seed <= 100; ++seed) {
        paxos::World w(seed);
        w.verbose = false;
        w.randomDelays(4, 6);
        w.setRetry(fixed, randomExtra);
        w.propose(11, "A");
        w.runUntil(5);
        if (two) {
            w.propose(12, "B");
        }
        for (sim::Time t = 10; t <= 1000 && !w.someProposerKnows(); t += 10) {
            w.runUntil(t);
        }
        if (w.someProposerKnows()) {
            ++decided;
            slowest = std::max(slowest, w.now());
        }
        ballots += w.ballotsTried();
    }
    std::printf("%-33s a proposer learned the value within 1000 ms in %3d of 100 seeds;\n"
                "%-33s ballots started: %4ld",
                label, decided, "", ballots);
    if (decided > 0) {
        std::printf("; slowest run: %lld ms", static_cast<long long>(slowest));
    }
    std::printf("\n");
}

int main()
{
    std::printf("--- the start of one duel (seed 1, fixed timeout 14 ms) ---\n");
    paxos::World w(1);
    w.randomDelays(4, 6);
    w.setRetry(14, 0);
    w.propose(11, "A");
    w.runUntil(5);
    w.propose(12, "B");
    w.runUntil(45);
    std::printf("--- statistics ---\n");
    study("two proposers, timeout 14 ms:", 14, 0);
    study("ONE proposer, timeout 14 ms:", 14, 0, false);
    study("two, timeout 14 + random 0-30 ms:", 14, 30);
    study("two proposers, timeout 40 ms:", 40, 0);
    return 0;
}
