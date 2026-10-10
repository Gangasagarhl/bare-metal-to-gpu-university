// m1_forensic_fix.cpp - MP6 starter lab, forensic answer key: two runs.
//   1. prevention alone: the WRONG map of m1_forensic.in, with the innovation gate switched on
//   2. the fix: post 4 re-surveyed in the map, gate on (m1_forensic_fix.in on stdin)
#include "mp6_stack.hpp"
#include <fstream>

namespace {

bool report(const char* title, const mp6::Config& cfg)
{
    const mp6::Report r = mp6::runMission(cfg, nullptr, false);
    std::printf("%s\n", title);
    for (const mp6::LegResult& l : r.legs) { std::printf("  %-7s true stop error %.3f m\n", l.name.c_str(), l.trueErr); }
    mp6::printPosts(cfg, r);
    bool ok = true;
    for (const mp6::Check& c : mp6::nominalChecks(cfg, r)) {
        std::printf("  %s %s  [%s]\n", c.id.c_str(), c.pass ? "PASS" : "FAIL", c.value.c_str());
        ok = ok && c.pass;
    }
    std::printf("\n");
    return ok;
}

}  // namespace

int main()
{
    std::ifstream f("m1_forensic.in");
    mp6::Config gated = mp6::readConfig(f);
    gated.ekf.nisGate = 9.21;
    const bool a = report("run 1: wrong map, innovation gate 9.21", gated);
    const bool b = report("run 2: post 4 re-surveyed in the map, gate 9.21", mp6::readConfig(std::cin));
    std::printf("prevention alone: %s; fix: %s\n", a ? "PASS" : "FAIL", b ? "PASS" : "FAIL");
    return b ? 0 : 1;
}
