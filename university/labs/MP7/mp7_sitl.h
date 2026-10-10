// mp7_sitl.h - MP7 starter SITL harness. Flies one scenario in the DN401 course simulator
// (labs/F10-33/dronesim.hpp) with a guard module plugged in, injects sensor faults, and
// returns what happened. The integration rule lives here (the "commander" part):
//   first guard fault -> failsafe LAND, logged as "estimator unhealthy: <faults>", and the
//   horizontal command is zeroed (descend without trusting position or heading; drift with
//   the wind). This is the course's design choice, argued in the MP7 handbook.
// Teaching model only: it is NOT PX4 and NOT ArduPilot.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../F10-33/dronesim.hpp"
#include "est_guard.h"

namespace mp7 {

struct Scenario
{
    std::string id, what;
    std::uint64_t seed = 42;
    dn::V3 wind{1.0, 0.5, 0};
    double mag_int = 0.004, mag_dir = 0.7;  // benign current-dependent compass interference
    std::vector<dn::V3> mission;
    bool guard_on = true;
    double fence_radius = 100;
    double batt_mah = 5000;  // the pack flown, and what the flight software is told
    double jump_t = -1;  // from this time on, add jump to the horizontal GNSS position
    dn::V3 jump;
    double freeze_t = -1;  // from this time on, the horizontal GNSS data stop changing
    double mag_bias_deg = 0;  // compass mounted rotated by this angle (field strength unchanged)
    double t_max = 400;
};

struct TraceRow
{
    double t, speed, pos_innov, mag_dev;
    std::string mode;
};

struct Result
{
    std::vector<dn::Event> events;
    unsigned faults = kNone;
    double fault_t = -1;
    bool mission_done = false, landed = false, crashed = false;
    double max_dist = 0, land_dist_home = 0, drift_after_fault = 0, t_end = 0;
    double max_innov = 0, max_mag_dev = 0;
    std::vector<TraceRow> trace;  // one row per 0.5 s
};

inline std::vector<dn::V3> laps(int n, double r)
{
    std::vector<dn::V3> m;
    for (int i = 0; i < n; ++i) {
        m.push_back({r, 0, 10});
        m.push_back({r, r, 10});
        m.push_back({0, r, 10});
        m.push_back({0, 0, 10});
    }
    return m;
}

template <class Guard>
Result fly(const Scenario& sc, const GuardParams& gp = GuardParams{})
{
    dn::World w;
    w.rng = dn::Rng(sc.seed);
    w.wind = sc.wind;
    w.mag_int_per_amp = sc.mag_int;
    w.mag_int_dir = sc.mag_dir;
    dn::Fsw f;
    f.p.fence_radius = sc.fence_radius;
    w.batt.capacity_mah = sc.batt_mah;
    f.p.batt_capacity_mah = sc.batt_mah;
    Guard g(gp);
    Result r;

    dn::Sensors s = w.sense();
    g.reset_on_ground(s);
    f.arm(s, sc.mission);
    dn::V3 frozen_pos, frozen_vel, fault_pos;
    bool frozen = false, reacted = false;
    int k = 0;
    while (w.t < sc.t_max) {
        s = w.sense();
        s.mag_heading = dn::wrap(s.mag_heading + sc.mag_bias_deg * dn::kPi / 180.0);
        if (sc.jump_t >= 0 && s.t >= sc.jump_t) {
            s.gps_pos.x += sc.jump.x;
            s.gps_pos.y += sc.jump.y;
        }
        if (sc.freeze_t >= 0 && s.t >= sc.freeze_t) {
            if (!frozen) {
                frozen = true;
                frozen_pos = s.gps_pos;
                frozen_vel = s.gps_vel;
            }
            s.gps_pos.x = frozen_pos.x;  // altitude keeps working, as a barometer would
            s.gps_pos.y = frozen_pos.y;
            s.gps_vel.x = frozen_vel.x;
            s.gps_vel.y = frozen_vel.y;
        }
        if (sc.guard_on) {
            const GuardStatus gs = g.update(s);
            r.max_innov = std::max(r.max_innov, gs.pos_innov);
            r.max_mag_dev = std::max(r.max_mag_dev, gs.mag_dev);
            if (gs.faults != kNone && !reacted) {
                reacted = true;
                r.faults = gs.faults;
                r.fault_t = s.t;
                fault_pos = w.pos;
                f.failsafe(dn::Mode::Land, s.t, "estimator unhealthy: " + fault_names(gs.faults));
            }
            if (k % 50 == 0)
                r.trace.push_back({s.t, dn::hnorm(s.gps_vel), gs.pos_innov, gs.mag_dev, dn::name(f.mode)});
        }
        dn::Command cmd = f.step(s);
        if (reacted) {
            cmd.acc.x = 0;
            cmd.acc.y = 0;
        }
        w.step(cmd);
        ++k;
        r.max_dist = std::max(r.max_dist, dn::hnorm(w.pos));
        if (f.mode == dn::Mode::Disarmed || w.crashed) break;
    }
    r.events = f.events;
    for (const auto& e : f.events)
        if (e.text.find("mission complete") != std::string::npos) r.mission_done = true;
    r.crashed = w.crashed;
    r.landed = f.mode == dn::Mode::Disarmed && w.on_ground && !w.crashed;
    r.land_dist_home = dn::hnorm(w.pos);
    if (reacted) r.drift_after_fault = dn::hnorm(w.pos - fault_pos);
    r.t_end = w.t;
    return r;
}

}  // namespace mp7
