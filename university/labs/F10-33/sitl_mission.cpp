// sitl_mission.cpp - software in the loop with the course simulator: the flight software
// (dn::Fsw) runs against a simulated world (dn::World) in lockstep, flies a square
// mission, and is judged by acceptance tests. Then the same flight is repeated to show
// that a lockstep simulation is deterministic.
#include "dronesim.hpp"

#include <cstdio>
#include <vector>

struct Result
{
    bool crashed = false, landed = false;
    int waypoints = 0;
    double max_alt = 0, land_err = 0, used_mah = 0, t_end = 0;
    dn::V3 final_pos;
};

Result fly(bool print)
{
    dn::World w;
    w.wind = {1.0, 0.5, 0};  // constant wind, m/s
    dn::Fsw f;
    const std::vector<dn::V3> square = {{20, 0, 10}, {20, 20, 10}, {0, 20, 10}};
    Result r;

    dn::Sensors s = w.sense();
    for (const auto& why : f.prearm(s)) std::printf("pre-arm: %s\n", why.c_str());
    f.arm(s, square);
    if (print)
        std::printf("%6s %-8s %7s %7s %6s %6s %6s\n", "t[s]", "mode", "x[m]", "y[m]", "z[m]", "V",
                    "A");
    int k = 0;
    while (w.t < 300.0) {
        s = w.sense();              // 1. the simulator produces sensor data
        dn::Command c = f.step(s);  // 2. the flight software computes one step
        w.step(c);                  // 3. the simulator advances by exactly one step
        r.max_alt = std::max(r.max_alt, w.pos.z);
        if (print && k % 500 == 0)
            std::printf("%6.1f %-8s %7.2f %7.2f %6.2f %6.2f %6.1f\n", w.t, dn::name(f.mode),
                        w.pos.x, w.pos.y, w.pos.z, s.volts, s.amps);
        ++k;
        if (f.mode == dn::Mode::Disarmed || w.crashed) break;
    }
    for (const auto& e : f.events) {
        if (print) std::printf("event %7.2f s  %s\n", e.t, e.text.c_str());
        if (e.text.rfind("waypoint", 0) == 0) ++r.waypoints;
    }
    r.crashed = w.crashed;
    r.landed = f.mode == dn::Mode::Disarmed && w.on_ground;
    r.land_err = dn::hnorm(w.pos - f.home);
    r.used_mah = w.batt.used_mah;
    r.t_end = w.t;
    r.final_pos = w.pos;
    return r;
}

int main()
{
    const Result a = fly(true);
    std::printf("\nacceptance tests\n");
    int fails = 0;
    auto check = [&](const char* name, bool ok, double value) {
        std::printf("  %-44s %-4s (%.2f)\n", name, ok ? "PASS" : "FAIL", value);
        fails += ok ? 0 : 1;
    };
    check("A1 all 3 waypoints reached", a.waypoints == 3, a.waypoints);
    check("A2 max altitude <= 16.0 m (RTL 15 m + 1 m)", a.max_alt <= 16.0, a.max_alt);
    check("A3 landed and disarmed", a.landed, a.t_end);
    check("A4 landing point within 1.5 m of home", a.land_err <= 1.5, a.land_err);
    check("A5 no crash", !a.crashed, a.used_mah);

    const Result b = fly(false);  // same seed, same inputs: must be bit-identical
    const bool same = a.final_pos.x == b.final_pos.x && a.final_pos.y == b.final_pos.y &&
                      a.t_end == b.t_end && a.used_mah == b.used_mah;
    check("A6 second run bit-identical (lockstep)", same, b.t_end);
    std::printf("battery used %.0f mAh of 5000; flight time %.2f s\n", a.used_mah, a.t_end);
    return fails == 0 ? 0 : 1;
}
