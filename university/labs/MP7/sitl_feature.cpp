// sitl_feature.cpp - MP7 milestone 1, step 2: the estimator guard flying in SITL (the DN401
// course simulator). Every scenario states its expectation BEFORE it runs; controls (guard
// off) prove that each injected fault really is harmful; K rows record a known gap.
// Writes sitl_results.csv for the test report (report.py).
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "mp7_sitl.h"

using mp7::Result;
using mp7::Scenario;

struct Row
{
    Scenario sc;
    std::string kind;  // TEST, CONTROL (guard off; the fault must do harm) or GAP
    std::string expect;
    double inject_t;
    std::function<bool(const Result&)> ok;
};

int main()
{
    std::vector<dn::V3> logA;  // the F10-38 Log A mission: 15 x (10 m out, back)
    for (int i = 0; i < 15; ++i) {
        logA.push_back({10, 0, 10});
        logA.push_back({0, 0, 10});
    }
    std::vector<Row> rows;
    for (int i = 1; i <= 5; ++i) {
        Scenario s;
        s.id = "N" + std::to_string(i);
        s.what = "nominal laps, seed " + std::to_string(i) + ", wind " + std::to_string(i) + " of 5";
        s.seed = static_cast<std::uint64_t>(i);
        s.wind = {1.0 + 0.3 * i, -0.5 + 0.2 * i, 0};
        s.mission = mp7::laps(2, 40);
        rows.push_back({s, "TEST", "no fault; mission done; lands < 2 m from home", 0,
                        [](const Result& r) {
                            return r.faults == mp7::kNone && r.mission_done && r.landed &&
                                   r.land_dist_home < 2.0;
                        }});
    }
    Scenario m;
    m.id = "M0";
    m.what = "Log A compass interference, guard OFF";
    m.mag_int = 0.049;
    m.mag_dir = 176.0 * dn::kPi / 180.0;
    m.wind = {1.5, 0.5, 0};
    m.batt_mah = 1500;
    m.mission = logA;
    m.guard_on = false;
    rows.push_back({m, "CONTROL", "harm reproduced: vehicle > 100 m from home", 0,
                    [](const Result& r) { return r.max_dist > 100.0; }});
    m.id = "M1";
    m.what = "Log A compass interference, guard ON";
    m.guard_on = true;
    rows.push_back({m, "TEST", "MAG_FIELD <= 1.0 s after arming; lands; never > 5 m out", 0,
                    [](const Result& r) {
                        return (r.faults & mp7::kMagField) && r.fault_t <= 1.0 && r.landed &&
                               r.max_dist < 5.0;
                    }});
    Scenario g;
    g.mission = mp7::laps(2, 40);
    g.id = "G0";
    g.what = "GNSS frozen at 20 s, guard OFF";
    g.freeze_t = 20;
    g.guard_on = false;
    rows.push_back({g, "CONTROL", "harm reproduced: vehicle > 200 m from home", 20,
                    [](const Result& r) { return r.max_dist > 200.0; }});
    g.id = "G1";
    g.what = "GNSS frozen at 20 s, guard ON";
    g.guard_on = true;
    rows.push_back({g, "TEST", "GNSS_STALE <= 0.55 s after; lands; drift <= 30 m", 20,
                    [](const Result& r) {
                        return (r.faults & mp7::kGnssStale) && r.fault_t - 20 <= 0.55 && r.landed &&
                               r.drift_after_fault <= 30.0;
                    }});
    g.id = "G2";
    g.what = "GNSS jump 15 m east at 20 s, guard ON";
    g.freeze_t = -1;
    g.jump_t = 20;
    g.jump = {0, 15, 0};
    rows.push_back({g, "TEST", "GNSS_JUMP <= 0.35 s after; lands; drift <= 30 m", 20,
                    [](const Result& r) {
                        return (r.faults & mp7::kGnssJump) && r.fault_t - 20 <= 0.35 && r.landed &&
                               r.drift_after_fault <= 30.0;
                    }});
    g.id = "G3";
    g.what = "GNSS jump 1 m east at 20 s (inside gate), guard ON";
    g.jump = {0, 1, 0};
    rows.push_back({g, "TEST", "no fault; mission done; lands", 20,
                    [](const Result& r) {
                        return r.faults == mp7::kNone && r.mission_done && r.landed;
                    }});
    Scenario k;
    k.id = "K1";
    k.what = "compass mounted 70 deg rotated, field normal, guard ON";
    k.mission = mp7::laps(2, 40);
    k.mag_bias_deg = 70;
    rows.push_back({k, "GAP", "known gap: no heading check, so no fault (see landed)", 0,
                    [](const Result& r) { return r.faults == mp7::kNone; }});

    std::FILE* csv = std::fopen("sitl_results.csv", "w");
    if (!csv) {
        std::perror("sitl_results.csv");
        return 2;
    }
    std::fprintf(csv, "id,kind,what,expect,faults,latency_s,mission_done,landed,crashed,max_dist_m,"
                      "drift_m,max_innov_m,max_mag_dev,t_end_s,verdict\n");
    int fails = 0;
    std::printf("%-3s %-8s %-52s %-11s %7s %6s %7s %6s %s\n", "id", "kind", "scenario", "fault",
                "lat[s]", "landed", "max[m]", "drift", "verdict");
    for (const Row& row : rows) {
        const Result r = mp7::fly<mp7::EstGuard>(row.sc);
        const bool ok = row.ok(r);
        std::string verdict = ok ? "PASS" : "FAIL";
        if (row.kind == "GAP") verdict = ok ? "GAP-CONFIRMED" : "GAP-CHANGED";
        if (!ok) ++fails;
        const double lat = r.fault_t < 0 ? -1 : r.fault_t - row.inject_t;
        const std::string fn = mp7::fault_names(r.faults);
        std::printf("%-3s %-8s %-52s %-11s %7.2f %6s %7.1f %6.1f %s\n", row.sc.id.c_str(),
                    row.kind.c_str(), row.sc.what.c_str(), fn.c_str(), lat,
                    r.landed ? "yes" : (r.crashed ? "CRASH" : "no"), r.max_dist, r.drift_after_fault,
                    verdict.c_str());
        std::printf("    expect: %s\n", row.expect.c_str());
        std::string modes;
        for (const auto& e : r.events)
            if (e.text.find(" -> ") != std::string::npos) {
                char b[160];
                std::snprintf(b, sizeof b, "%s%.2f %s", modes.empty() ? "" : " | ", e.t,
                              e.text.c_str());
                modes += b;
            }
        if (modes.size() > 150) modes = modes.substr(0, 147) + "...";
        std::printf("    events: %s\n", modes.c_str());
        std::fprintf(csv, "%s,%s,\"%s\",\"%s\",%s,%.2f,%d,%d,%d,%.1f,%.1f,%.2f,%.3f,%.2f,%s\n",
                     row.sc.id.c_str(), row.kind.c_str(), row.sc.what.c_str(), row.expect.c_str(),
                     fn.c_str(), lat, r.mission_done, r.landed, r.crashed, r.max_dist,
                     r.drift_after_fault, r.max_innov, r.max_mag_dev, r.t_end, verdict.c_str());
    }
    std::fclose(csv);
    std::printf("%d scenario(s) did not meet the expectation written before the run\n", fails);
    return fails == 0 ? 0 : 1;
}
