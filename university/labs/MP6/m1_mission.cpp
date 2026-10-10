// m1_mission.cpp - MP6 starter lab, milestone M1: one delivery run of the simulated stack.
// Reads the configuration on stdin (m1_mission.in), prints the event log and the acceptance
// checks, and writes the run's log in the fixed schema to m1_log.csv.
#include "mp6_stack.hpp"

int main()
{
    const mp6::Config cfg = mp6::readConfig(std::cin);
    std::FILE* log = std::fopen("m1_log.csv", "w");
    const mp6::Report r = mp6::runMission(cfg, log, true);
    if (log) { std::fclose(log); }
    std::printf("\nlocalisation posts (what the filter saw):\n");
    mp6::printPosts(cfg, r);
    std::printf("\nacceptance checks (lab plan values):\n");
    bool ok = true;
    for (const mp6::Check& c : mp6::nominalChecks(cfg, r)) {
        std::printf("  %s %-58s %s  [%s]\n", c.id.c_str(), c.what.c_str(), c.pass ? "PASS" : "FAIL", c.value.c_str());
        ok = ok && c.pass;
    }
    std::printf("M1 run: %s (log: m1_log.csv, %zu event lines)\n", ok ? "PASS" : "FAIL", r.events.size());
    return ok ? 0 : 1;
}
