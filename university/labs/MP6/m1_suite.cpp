// m1_suite.cpp - MP6 starter lab, milestone M1: the acceptance suite.
//   part A: the nominal delivery on ten seeds (checks T1-T4)
//   part B: three safety scenarios: e-stop (E1-E5), stalled navigator (W1-W4), misconfigured speed (T1, T4)
//   part C: mutants - deliberate faults; a suite that cannot see them is not evidence
// The base configuration is m1_mission.in. Exit code 0 only if A and B pass and every
// required mutant is killed.
#include "mp6_stack.hpp"
#include <fstream>

namespace {

mp6::Config base()
{
    std::ifstream f("m1_mission.in");
    return mp6::readConfig(f);
}

mp6::Config estopScenario(mp6::Config c)
{
    c.estopPress = 20.0;      // while driving leg 2
    c.estopRelease = 23.0;
    c.resets = {22.0, 26.0};  // the first reset comes while the button is still pressed
    return c;
}

mp6::Config stallScenario(mp6::Config c)
{
    c.stallAt = 30.0;         // the navigator produces nothing for 2 s
    c.stallFor = 2.0;
    c.resets = {34.0};
    return c;
}

mp6::Config speedScenario(mp6::Config c)
{
    c.navMaxVel = 0.45;       // the navigator is misconfigured; the supervisor must hold the limit
    return c;
}

std::string fmt(const char* f, double a, double b = 0.0)
{
    char buf[96];
    std::snprintf(buf, sizeof buf, f, a, b);
    return buf;
}

std::vector<mp6::Check> estopChecks(const mp6::Config& c, const mp6::Report& r)
{
    const double p = c.estopPress;
    const double last = c.resets.back();
    const bool t1 = mp6::nominalChecks(c, r)[0].pass;
    return {
        {"E1", "zero command within one supervisor cycle (10 ms)", r.tZeroCmd >= 0 && r.tZeroCmd - p <= 0.0100001,
         fmt("%.3f s", r.tZeroCmd - p)},
        {"E2", "wheels still within 0.6 s of the press", r.tStopped >= 0 && r.tStopped - p <= 0.6, fmt("%.2f s", r.tStopped - p)},
        {"E3", "stopping distance <= 0.12 m", r.tStopped >= 0 && r.stopDist <= 0.12, fmt("%.3f m", r.stopDist)},
        {"E4", "no motion from press + 0.6 s until the accepted reset", r.travelBetween(p + 0.6, last) <= 0.005,
         fmt("%.3f m moved", r.travelBetween(p + 0.6, last))},
        {"E5", "task completes after the reset", t1, fmt("end %.1f s", r.tEnd)},
    };
}

std::vector<mp6::Check> stallChecks(const mp6::Config& c, const mp6::Report& r)
{
    const double s = c.stallAt;
    const double w = c.safety.watchdog;
    const bool t1 = mp6::nominalChecks(c, r)[0].pass;
    return {
        {"W1", "safe stop within watchdog + 20 ms of the stall", r.tLock >= 0 && r.tLock - s <= w + 0.02, fmt("%.2f s", r.tLock - s)},
        {"W2", "wheels still within watchdog + 0.6 s", r.tStopped >= 0 && r.tStopped - s <= w + 0.6, fmt("%.2f s", r.tStopped - s)},
        {"W3", "no motion from then until the reset", r.travelBetween(s + w + 0.6, c.resets.back()) <= 0.005,
         fmt("%.3f m moved", r.travelBetween(s + w + 0.6, c.resets.back()))},
        {"W4", "task completes after the reset", t1, fmt("end %.1f s", r.tEnd)},
    };
}

std::vector<mp6::Check> speedChecks(const mp6::Config& c, const mp6::Report& r)
{
    const std::vector<mp6::Check> n = mp6::nominalChecks(c, r);
    return {n[0], n[3]};
}

bool show(const std::string& title, const std::vector<mp6::Check>& cs)
{
    bool ok = true;
    std::printf("%s\n", title.c_str());
    for (const mp6::Check& c : cs) {
        std::printf("  %s %-52s %s  [%s]\n", c.id.c_str(), c.what.c_str(), c.pass ? "PASS" : "FAIL", c.value.c_str());
        ok = ok && c.pass;
    }
    return ok;
}

// Every check of every scenario for one configuration; returns the ids that failed.
std::string failedIds(const mp6::Config& b)
{
    std::string ids;
    auto collect = [&](const std::vector<mp6::Check>& cs) {
        for (const mp6::Check& c : cs) { if (!c.pass) { ids += (ids.empty() ? "" : " ") + c.id; } }
    };
    collect(mp6::nominalChecks(b, mp6::runMission(b, nullptr, false)));
    const mp6::Config e = estopScenario(b);
    collect(estopChecks(e, mp6::runMission(e, nullptr, false)));
    const mp6::Config s = stallScenario(b);
    collect(stallChecks(s, mp6::runMission(s, nullptr, false)));
    const mp6::Config v = speedScenario(b);
    const std::vector<mp6::Check> sc = speedChecks(v, mp6::runMission(v, nullptr, false));
    for (const mp6::Check& c : sc) { if (!c.pass) { ids += (ids.empty() ? "" : " ") + ("S3:" + c.id); } }
    return ids;
}

}  // namespace

int main()
{
    bool ok = true;
    // A. nominal task on ten seeds
    std::printf("A. nominal delivery, seeds 1-10\n");
    std::printf("seed  end s  worst stop err  RMS err  max err  NEES  max v   T1 T2 T3 T4\n");
    int passed = 0;
    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        mp6::Config c = base();
        c.seed = seed;
        const mp6::Report r = mp6::runMission(c, nullptr, false);
        const std::vector<mp6::Check> cs = mp6::nominalChecks(c, r);
        double worst = 0.0;
        for (const mp6::LegResult& l : r.legs) { worst = std::fmax(worst, l.reached ? l.trueErr : 99.0); }
        bool all = true;
        for (const mp6::Check& k : cs) { all = all && k.pass; }
        passed += all ? 1 : 0;
        std::printf("%4llu %6.1f %15.3f %8.3f %8.3f %5.2f %6.3f   %s %s %s %s\n", static_cast<unsigned long long>(seed), r.tEnd,
                    worst, r.rmsErr, r.maxErr, r.meanNees, r.maxSpeed, cs[0].pass ? "ok" : "--", cs[1].pass ? "ok" : "--",
                    cs[2].pass ? "ok" : "--", cs[3].pass ? "ok" : "--");
    }
    std::printf("task success: %d of 10 (required: 10 of 10)\n\n", passed);
    ok = ok && passed == 10;

    // B. safety scenarios on seed 1
    const mp6::Config e = estopScenario(base());
    const mp6::Report er = mp6::runMission(e, nullptr, false);
    for (const std::string& ev : er.events) {
        if (ev.find("supervisor") != std::string::npos) { std::printf("    %s\n", ev.c_str()); }
    }
    ok = show("B1. e-stop pressed at 20.0 s, released at 23.0 s, reset at 22.0 s and 26.0 s", estopChecks(e, er)) && ok;
    const mp6::Config s = stallScenario(base());
    const mp6::Report sr = mp6::runMission(s, nullptr, false);
    for (const std::string& ev : sr.events) {
        if (ev.find("supervisor") != std::string::npos) { std::printf("    %s\n", ev.c_str()); }
    }
    ok = show("B2. navigator silent from 30.0 s to 32.0 s, reset at 34.0 s", stallChecks(s, sr)) && ok;
    const mp6::Config v = speedScenario(base());
    ok = show("B3. navigator misconfigured to 0.45 m/s", speedChecks(v, mp6::runMission(v, nullptr, false))) && ok;

    // C. mutants
    std::printf("\nC. mutants (each re-runs A seed 1 and B1-B3)\n");
    struct M { const char* name; bool required; };
    const M mutants[] = {{"no_speed_clamp", true}, {"no_watchdog", true}, {"estop_unlatched", true},
                         {"bearing_sign", true}, {"radius_plus_2pc", false}};
    int killed = 0, required = 0;
    for (const M& m : mutants) {
        mp6::Config c = base();
        c.mutant = m.name;
        const std::string ids = failedIds(c);
        const bool k = !ids.empty();
        std::printf("  %-16s %-9s %s %s\n", m.name, m.required ? "required" : "info", k ? "KILLED by" : "SURVIVED",
                    k ? ids.c_str() : "(no check failed)");
        if (m.required) {
            ++required;
            killed += k ? 1 : 0;
        }
    }
    std::printf("required mutants killed: %d of %d\n", killed, required);
    ok = ok && killed == required;
    std::printf("\nM1 suite: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
