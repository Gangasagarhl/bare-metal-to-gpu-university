// m1_forensic.cpp - MP6 starter lab, forensic case "short of the table".
// Runs the configuration of the failing delivery (m1_forensic.in on stdin) and prints the
// evidence pack: event log, the filter's per-post statistics, a log excerpt and the checks.
#include "mp6_stack.hpp"
#include <fstream>

int main()
{
    const mp6::Config cfg = mp6::readConfig(std::cin);
    std::FILE* log = std::fopen("m1_forensic_log.csv", "w");
    const mp6::Report r = mp6::runMission(cfg, log, true);
    if (log) { std::fclose(log); }
    std::printf("\nevidence 2: localisation posts (what the filter saw)\n");
    mp6::printPosts(cfg, r);
    std::printf("\nevidence 3: log excerpt, one row per second from 6 s to 15 s (columns of the log schema)\n");
    std::ifstream in("m1_forensic_log.csv");
    std::string line;
    std::getline(in, line);
    std::printf("%s\n", line.c_str());
    while (std::getline(in, line)) {
        const double t = std::stod(line);
        const double whole = std::round(t);
        if (t >= 6.0 && t <= 15.0 && std::fabs(t - whole) < 1e-6) { std::printf("%s\n", line.c_str()); }
    }
    std::printf("\nevidence 4: acceptance checks\n");
    bool ok = true;
    for (const mp6::Check& c : mp6::nominalChecks(cfg, r)) {
        std::printf("  %s %-58s %s  [%s]\n", c.id.c_str(), c.what.c_str(), c.pass ? "PASS" : "FAIL", c.value.c_str());
        ok = ok && c.pass;
    }
    std::printf("forensic run: %s (expected: FAIL; this is the incident)\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
