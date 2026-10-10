// guard_tests.h - MP7 unit tests for an estimator guard, written once and run against any
// guard class with the same interface (ours in guard_unit.cpp, Team B's in the forensic lab).
// Each test feeds a synthetic sensor stream (deterministic noise) and checks the verdict.
#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "est_guard.h"

namespace mp7 {

struct Stream  // a synthetic vehicle moving at constant velocity, sensors at rate 1/dt
{
    double t = 0, dt = 0.01;
    dn::V3 pos, vel, jump;
    double mag = 1.0;
    bool frozen = false;
    dn::Sensors last;
    dn::Rng rng{7};

    dn::Sensors next()
    {
        t += dt;
        pos = pos + dt * vel;
        if (frozen) {
            last.t = t;
            last.mag_norm = mag;
            return last;
        }
        dn::Sensors s;
        s.t = t;
        s.gps_pos = pos + jump + dn::V3{0.15 * rng.gauss(), 0.15 * rng.gauss(), 0.10 * rng.gauss()};
        s.gps_vel = vel + dn::V3{0.05 * rng.gauss(), 0.05 * rng.gauss(), 0.05 * rng.gauss()};
        s.mag_norm = mag;
        last = s;
        return s;
    }
};

struct Verdict
{
    bool pass;
    std::string detail;
};

template <class Guard>
struct Run  // a guard fed by a stream; helpers to run for some time and read the result
{
    Stream st;
    Guard g;
    GuardStatus gs;
    explicit Run(GuardParams p = GuardParams{}, double dt = 0.01) : g(p)
    {
        st.dt = dt;
        g.reset_on_ground(st.next());
    }
    void for_s(double secs)
    {
        const double end = st.t + secs;
        while (st.t < end - 1e-9) gs = g.update(st.next());
    }
};

template <class Guard>
std::vector<std::pair<std::string, std::function<Verdict()>>> guard_tests()
{
    using R = Run<Guard>;
    auto fmt = [](const char* f, double a, double b = 0) {
        char buf[96];
        std::snprintf(buf, sizeof buf, f, a, b);
        return std::string(buf);
    };
    return {
        {"U1 hover 60 s, healthy sensors -> no fault",
         [=] {
             R r;
             r.for_s(60);
             return Verdict{r.gs.faults == kNone, fmt("innov at end %.2f m", r.gs.pos_innov)};
         }},
        {"U2 straight line at 5 m/s for 60 s -> no fault",
         [=] {
             R r;
             r.st.vel = {5, 0, 0};
             r.for_s(60);
             return Verdict{r.gs.faults == kNone,
                            fault_names(r.gs.faults) + fmt(", innov %.2f m", r.gs.pos_innov)};
         }},
        {"U3 field strength 0.6 for 0.40 s (shorter than hold) -> no fault",
         [=] {
             R r;
             r.for_s(1);
             r.st.mag = 0.6;
             r.for_s(0.40);
             r.st.mag = 1.0;
             r.for_s(2);
             return Verdict{r.gs.faults == kNone, fault_names(r.gs.faults)};
         }},
        {"U4 field strength 0.6 held -> MAG_FIELD after 0.50 s",
         [=] {
             R r;
             r.for_s(1);
             const double t0 = r.st.t;
             r.st.mag = 0.6;
             r.for_s(1);
             const double lat = r.gs.first_fault_t - t0;
             return Verdict{r.gs.faults == kMagField && lat > 0.49 && lat < 0.52,
                            fault_names(r.gs.faults) + fmt(", latency %.2f s", lat)};
         }},
        {"U5 GNSS jump of 10 m -> GNSS_JUMP within 0.35 s",
         [=] {
             R r;
             r.for_s(5);
             const double t0 = r.st.t;
             r.st.jump = {0, 10, 0};
             r.for_s(2);
             const double lat = r.gs.first_fault_t - t0;
             return Verdict{(r.gs.faults & kGnssJump) && lat <= 0.35,
                            fault_names(r.gs.faults) + fmt(", latency %.2f s", lat)};
         }},
        {"U6 GNSS jump of 1 m (inside the gate) -> no fault",
         [=] {
             R r;
             r.for_s(5);
             r.st.jump = {0, 1, 0};
             r.for_s(10);
             return Verdict{r.gs.faults == kNone, fault_names(r.gs.faults)};
         }},
        {"U7 GNSS frozen -> GNSS_STALE after 0.50 s",
         [=] {
             R r;
             r.st.vel = {2, 0, 0};
             r.for_s(5);
             const double t0 = r.st.t;
             r.st.frozen = true;
             r.for_s(2);
             const double lat = r.gs.first_fault_t - t0;
             return Verdict{(r.gs.faults & kGnssStale) && lat > 0.49 && lat < 0.53,
                            fault_names(r.gs.faults) + fmt(", latency %.2f s", lat)};
         }},
        {"U8 fault stays latched after the input recovers",
         [=] {
             R r;
             r.for_s(1);
             r.st.mag = 0.5;
             r.for_s(1);
             r.st.mag = 1.0;
             r.for_s(10);
             return Verdict{r.gs.faults == kMagField, fault_names(r.gs.faults)};
         }},
        {"U9 EG_ENABLE = 0 -> never a fault, even with bad inputs",
         [=] {
             GuardParams p;
             p.enable = false;
             R r(p);
             r.for_s(1);
             r.st.mag = 0.3;
             r.st.jump = {10, 0, 0};
             r.for_s(5);
             return Verdict{r.gs.faults == kNone, fault_names(r.gs.faults)};
         }},
        {"U10 parameter check: 3 bad values -> 3 errors; defaults -> 0",
         [=] {
             GuardParams bad;
             bad.mag_tol = 0;
             bad.pos_gate_m = -1;
             bad.filt_tau_s = 0;
             const auto e = bad.validate();
             const auto ok = GuardParams{}.validate();
             return Verdict{e.size() == 3 && ok.empty(),
                            fmt("%.0f errors, defaults %.0f errors", static_cast<double>(e.size()),
                                static_cast<double>(ok.size()))};
         }},
        {"U11 sensors at 200 Hz, 3 m/s for 60 s -> no fault",
         [=] {
             R r(GuardParams{}, 0.005);
             r.st.vel = {3, 0, 0};
             r.for_s(60);
             return Verdict{r.gs.faults == kNone,
                            fault_names(r.gs.faults) + fmt(", innov %.2f m", r.gs.pos_innov)};
         }},
        {"U12 two faults: both latched, time of the first kept",
         [=] {
             R r;
             r.for_s(1);
             r.st.mag = 0.5;
             r.for_s(1);
             const double t_mag = r.gs.first_fault_t;
             r.st.jump = {0, 10, 0};
             r.for_s(1);
             return Verdict{r.gs.faults == (kMagField | kGnssJump) && r.gs.first_fault_t == t_mag,
                            fault_names(r.gs.faults) + fmt(", first at %.2f s", t_mag)};
         }},
    };
}

// Runs the tests whose id (first word) is in `only` (all tests if `only` is empty).
template <class Guard>
int run_suite(const char* title, const std::set<std::string>& only = {})
{
    std::printf("%s\n", title);
    int fails = 0, n = 0;
    for (const auto& [name, fn] : guard_tests<Guard>()) {
        if (!only.empty() && !only.count(name.substr(0, name.find(' ')))) continue;
        const Verdict v = fn();
        ++n;
        fails += v.pass ? 0 : 1;
        std::printf("  %-66s %s  (%s)\n", name.c_str(), v.pass ? "PASS" : "FAIL", v.detail.c_str());
    }
    std::printf("%d of %d tests passed\n", n - fails, n);
    return fails;
}

}  // namespace mp7
