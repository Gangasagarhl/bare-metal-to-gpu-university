// two_leaders_truth.cpp - the answer key's ground truth for "Two leaders": the same run,
// merged in TRUE time order, with each node's wall clock and the Lamport time that the
// nodes would have had if they had kept Lamport clocks (they did not log them).
#include "two_leaders_sim.hpp"

int main()
{
    sim::Cluster cluster;
    const std::vector<sim::LogLine> lines = cluster.run();
    std::printf("%-12s  %-12s  %4s  %s\n", "true time", "wall clock", "L", "node and event");
    for (const sim::LogLine& l : lines) {
        if (l.trueMs >= 9000 && l.trueMs < 16500) {
            std::printf("%s  %s  %4d  n%d %s\n", sim::clock(l.trueMs + 32400000).c_str(),
                        sim::clock(l.wallMs).c_str(), l.lamport, l.node, l.text.c_str());
        }
    }
    return 0;
}
