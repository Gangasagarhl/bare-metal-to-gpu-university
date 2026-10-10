// failsafe_matrix.cpp - failsafe injection tests in SITL (course simulator).
// Each case configures the flight software, flies a mission, injects one or more faults
// at known times, and checks the reaction against a written expectation:
// which mode, within which deadline, and how the flight ended.
#include "../F10-33/dronesim.hpp"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

struct Outcome
{
    double fs_time = -1;  // first failsafe transition
    std::string fs_text;
    std::string path = "MISSION";  // the sequence of modes after take-off
    bool crashed = false, landed = false, went_back_down = false;
    double dist_home = 0, real_left_pct = 0, max_dist = 0, t_end = 0;
};

struct Case
{
    std::string id, what;
    std::function<void(dn::World&, dn::Params&)> setup;
    std::function<void(double, dn::World&)> inject;
    double inject_t;          // when the (first) fault is injected, s
    std::string expect_text;  // substring of the first failsafe event
    double deadline;          // s after inject_t
    std::function<bool(const Outcome&)> end_ok;
    std::string end_rule;
};

std::vector<dn::V3> laps(int n, double r)
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

Outcome fly(const Case& c, const std::vector<dn::V3>& mission)
{
    dn::World w;
    w.wind = {1.0, 0.5, 0};
    dn::Fsw f;
    c.setup(w, f.p);
    w.batt.cells = f.p.cells;
    dn::Sensors s = w.sense();
    f.arm(s, mission);
    Outcome o;
    int last_rank = 0;
    while (w.t < 900.0) {
        c.inject(w.t, w);
        s = w.sense();
        const dn::Command cmd = f.step(s);
        w.step(cmd);
        o.max_dist = std::max(o.max_dist, dn::hnorm(w.pos));
        if (dn::rank(f.mode) < last_rank && f.mode != dn::Mode::Disarmed) o.went_back_down = true;
        last_rank = std::max(last_rank, dn::rank(f.mode));
        if (f.mode == dn::Mode::Disarmed || w.crashed) break;
    }
    for (const auto& e : f.events) {
        const auto arrow = e.text.find(" -> ");
        if (arrow != std::string::npos && e.text.rfind("TAKEOFF", arrow) == std::string::npos &&
            e.text.rfind("DISARMED -> ", 0) != 0)
            o.path += ">" + e.text.substr(arrow + 4, e.text.find(' ', arrow + 4) - arrow - 4);
        if (o.fs_time < 0 && e.text.find("failsafe") != std::string::npos) {
            o.fs_time = e.t;
            o.fs_text = e.text;
        }
    }
    o.crashed = w.crashed;
    o.landed = f.mode == dn::Mode::Disarmed && w.on_ground && !w.crashed;
    o.dist_home = dn::hnorm(w.pos);
    o.real_left_pct = 100.0 * w.batt.soc();
    o.t_end = w.t;
    return o;
}

int main()
{
    auto nothing = [](dn::World&, dn::Params&) {};
    auto rc_off_at = [](double t0) {
        return [t0](double t, dn::World& w) {
            if (t >= t0) w.rc_link = false;
        };
    };
    auto never = [](double, dn::World&) {};
    const auto home_ok = [](const Outcome& o) { return o.landed && o.dist_home < 2.0; };

    const std::vector<Case> cases = {
        {"F1", "RC loss in mission, action RTL", nothing, rc_off_at(30.0), 30.0,
         "RTL (failsafe: RC lost", 1.05, home_ok, "landed < 2 m from home"},
        {"F2", "RC loss in mission, action LAND",
         [](dn::World&, dn::Params& p) { p.rc_action = dn::Mode::Land; }, rc_off_at(30.0), 30.0,
         "LAND (failsafe: RC lost", 1.05,
         [](const Outcome& o) { return o.landed && o.dist_home > 5.0; },
         "landed where it was, not at home"},
        {"F3", "RC loss, action CONTINUE (mission)",
         [](dn::World&, dn::Params& p) { p.rc_action = dn::Mode::Mission; }, rc_off_at(30.0), 30.0,
         "failsafe ignored by setting (RC lost", 1.05, home_ok,
         "mission completes, then normal RTL"},
        {"F4", "battery low then critical, 1500 mAh pack",
         [](dn::World& w, dn::Params& p) {
             w.batt.capacity_mah = 1500;
             p.batt_capacity_mah = 1500;
         },
         never, 0.0, "RTL (failsafe: battery low", 900.0,
         [](const Outcome& o) { return o.landed && o.real_left_pct > 10.0; },
         "landed, > 10 % really left"},
        {"F5", "capacity set too high (2200 told, 1500 real), voltage backup on",
         [](dn::World& w, dn::Params& p) {
             w.batt.capacity_mah = 1500;
             p.batt_capacity_mah = 2200;
             p.batt_low_cell_v = 3.62;
         },
         never, 0.0, "RTL (failsafe: battery voltage low", 900.0,
         [](const Outcome& o) { return o.landed && !o.crashed; }, "landed, no crash"},
        {"F6", "geofence radius 50 m, mission leg to 70 m",
         [](dn::World&, dn::Params& p) { p.fence_radius = 50; }, never, 0.0,
         "RTL (failsafe: geofence", 900.0,
         [](const Outcome& o) { return o.landed && o.max_dist < 55.0; },
         "never beyond 55 m; landed home"},
        {"F7", "RC loss far out, then critical battery during RTL",
         [](dn::World& w, dn::Params& p) {
             w.batt.capacity_mah = 1500;
             p.batt_capacity_mah = 1500;
             p.batt_crit_pct = 25;
         },
         rc_off_at(190.0), 190.0, "RTL (failsafe: RC lost", 1.05,
         [](const Outcome& o) { return o.landed && !o.went_back_down; },
         "LAND after RTL, never back to RTL"},
    };

    std::printf("%-3s %-62s %8s %-34s %8s %-6s\n", "id", "case", "inject", "first failsafe event",
                "latency", "verdict");
    int fails = 0;
    for (const Case& c : cases) {
        std::vector<dn::V3> mission = laps(2, 40);
        if (c.id == "F4" || c.id == "F5") mission = laps(12, 40);
        if (c.id == "F6") mission = {{70, 0, 10}, {0, 0, 10}};
        if (c.id == "F7") mission = laps(6, 150);
        const Outcome o = fly(c, mission);
        const bool seen = o.fs_text.find(c.expect_text) != std::string::npos;
        const double latency = o.fs_time - c.inject_t;  // for F4-F6 inject_t = 0: time of flight
        const bool in_time = seen && latency <= c.deadline;
        const bool ok = in_time && c.end_ok(o);
        fails += ok ? 0 : 1;
        std::string shown = o.fs_text.empty() ? "(none)" : o.fs_text;
        if (shown.size() > 34) shown = shown.substr(0, 34);
        std::printf("%-3s %-62s %8.2f %-34s %8.2f %-6s\n", c.id.c_str(), c.what.c_str(), c.inject_t,
                    shown.c_str(), latency, ok ? "PASS" : "FAIL");
        std::printf(
            "    modes %s; landed %s, %.1f m from home, max %.1f m out, real charge left %.1f %%,"
            " t %.1f s [end rule: %s]\n",
            o.path.c_str(), o.landed ? "yes" : "no", o.dist_home, o.max_dist, o.real_left_pct,
            o.t_end, c.end_rule.c_str());
    }
    std::printf("%d of %zu cases failed\n", fails, cases.size());
    return fails == 0 ? 0 : 1;
}
