// two_leaders.cpp - prints the evidence pack of the forensic lab "Two leaders":
// the three nodes' logs, each stamped with that node's own wall clock. The excerpt covers
// the same eleven real seconds for every node (chosen by the simulator). Nothing else.
#include "two_leaders_sim.hpp"

int main()
{
    sim::Cluster cluster;
    const std::vector<sim::LogLine> lines = cluster.run();
    for (int node = 1; node <= 3; ++node) {
        std::printf("=== node%d.log ===\n", node);
        for (const sim::LogLine& l : lines) {
            if (l.node == node && l.trueMs >= 6000 && l.trueMs < 17000) {
                std::printf("%s n%d %s\n", sim::clock(l.wallMs).c_str(), node, l.text.c_str());
            }
        }
        std::printf("\n");
    }
    return 0;
}
