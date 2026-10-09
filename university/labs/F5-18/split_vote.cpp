// How many terms (elections) until the first leader? Equal timeouts versus
// randomized ones, measured over 200 seeds of the simulator for each setting.
#include <cstdio>
#include <map>

#include "cluster.h"

// Returns the term of the first leader, or 0 if none appeared within 3000 ms.
static int firstLeaderTerm(std::uint64_t seed, raft::Time lo, raft::Time hi, bool equal)
{
    raft::Options opt;
    opt.electionMin = lo;
    opt.electionMax = hi;
    if (equal) {
        opt.fixedTimeout = {{1, lo}, {2, lo}, {3, lo}, {4, lo}};
    }
    raft::Cluster c(4, seed, opt);  // 4 servers: a tie (2 votes against 2) is possible
    c.traceLevel = raft::Cluster::Trace::None;
    for (raft::Time t = 10; t <= 3000; t += 10) {
        c.runUntil(t);
        for (int id = 1; id <= 4; ++id) {
            if (c.node(id).role() == raft::Role::Leader) {
                return c.node(id).term();
            }
        }
    }
    return 0;
}

static void study(const char* label, raft::Time lo, raft::Time hi, bool equal)
{
    std::map<int, int> histogram;  // term of first leader -> number of seeds
    for (std::uint64_t seed = 1; seed <= 200; ++seed) {
        ++histogram[firstLeaderTerm(seed, lo, hi, equal)];
    }
    std::printf("%s\n", label);
    for (const auto& [term, count] : histogram) {
        if (term == 0) {
            std::printf("   no leader within 3000 ms : %3d of 200 seeds\n", count);
        } else {
            std::printf("   first leader in term %-4d : %3d of 200 seeds\n", term, count);
        }
    }
}

int main()
{
    study("equal timeouts, 150 ms for every server", 150, 150, true);
    study("random timeouts in 150-155 ms (narrow range)", 150, 155, false);
    study("random timeouts in 150-300 ms", 150, 300, false);
}
