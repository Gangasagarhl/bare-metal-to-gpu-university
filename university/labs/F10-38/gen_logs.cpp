// gen_logs.cpp - generates the two flight logs of the DN401 course forensic lab with the
// course simulator, and prints an excerpt of each. READ THIS FILE ONLY AFTER YOUR ANALYSIS:
// it contains the injected faults (the answer key of F10-38 explains them).
// Logs hold only what the flight software knew (no simulator truth), one row per 0.5 s.
#include "../F10-33/dronesim.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct Flight
{
    std::string file, title;
    dn::World w;
    dn::Fsw f;
    std::vector<dn::V3> mission;
};

void fly(Flight& fl)
{
    std::FILE* out = std::fopen(fl.file.c_str(), "w");
    if (!out) {
        std::perror(fl.file.c_str());
        return;
    }
    const dn::Params& p = fl.f.p;
    std::fprintf(out, "# %s\n", fl.title.c_str());
    std::fprintf(out,
                 "# params: cells=%d batt_capacity_mah=%.0f batt_low_pct=%.0f batt_crit_pct=%.0f "
                 "batt_low_cell_v=%.2f fence_radius=%.0f rtl_alt=%.0f rc_timeout=%.1f\n",
                 p.cells, p.batt_capacity_mah, p.batt_low_pct, p.batt_crit_pct, p.batt_low_cell_v,
                 p.fence_radius, p.rtl_alt, p.rc_timeout);
    dn::Sensors s = fl.w.sense();
    for (const auto& why : fl.f.prearm(s)) std::fprintf(out, "# pre-arm failed: %s\n", why.c_str());
    std::fprintf(out, "# pre-arm: mag_norm=%.3f volts=%.2f rc_valid=%d\n", s.mag_norm, s.volts,
                 s.rc_valid ? 1 : 0);
    fl.f.arm(s, fl.mission);
    std::fprintf(
        out, "t,mode,x,y,z,vx,vy,vz,acx_cmd,acy_cmd,yaw_est_deg,mag_hdg_deg,mag_norm,volts,amps,"
             "used_mah,rem_pct\n");
    int k = 0;
    while (fl.w.t < 600.0) {
        s = fl.w.sense();
        const dn::Command c = fl.f.step(s);
        if (k++ % 50 == 0) {
            const dn::V3 a = fl.f.acc_cmd_earth;
            std::fprintf(out,
                         "%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.3f,%.2f,%.1f,"
                         "%.0f,%.1f\n",
                         s.t, dn::name(fl.f.mode), s.gps_pos.x, s.gps_pos.y, s.gps_pos.z,
                         s.gps_vel.x, s.gps_vel.y, s.gps_vel.z, a.x, a.y,
                         fl.f.yaw_est * 180 / dn::kPi, s.mag_heading * 180 / dn::kPi, s.mag_norm,
                         s.volts, s.amps, fl.f.used_mah, fl.f.remaining_pct());
        }
        fl.w.step(c);
        if (fl.f.mode == dn::Mode::Disarmed || fl.w.crashed) break;
    }
    for (const auto& e : fl.f.events) std::fprintf(out, "# event %.2f %s\n", e.t, e.text.c_str());
    if (fl.w.crashed)
        std::fprintf(out, "# log ends: impact at %.2f s\n", fl.w.t);
    else
        std::fprintf(out, "# log ends: disarmed at %.2f s\n", fl.w.t);
    std::fclose(out);
}

int main()
{
    // Log A: hover-and-translate test with the new power wiring (team B, firmware v1.5).
    Flight a;
    a.file = "log_a_flyaway.csv";
    a.title =
        "Log A - team B, quad Q2, firmware v1.5 config 91be, hover-and-translate test (15 x 10 m)";
    a.w.wind = {1.5, 0.5, 0};
    a.w.batt.capacity_mah = 1500;
    a.w.mag_int_per_amp = 0.049;  // injected: motor current disturbs the compass
    a.w.mag_int_dir = 176.0 * dn::kPi / 180.0;
    a.f.p.batt_capacity_mah = 1500;
    a.f.p.fence_radius = 100;
    for (int i = 0; i < 15; ++i) {
        a.mission.push_back({10, 0, 10});
        a.mission.push_back({0, 0, 10});
    }
    fly(a);

    // Log B: survey laps with a new, smaller pack (team C, firmware v1.4).
    Flight b;
    b.file = "log_b_late_battery.csv";
    b.title = "Log B - team C, quad Q3, firmware v1.4 config 5a10, survey laps";
    b.w.wind = {1.0, -0.5, 0};
    b.w.batt.capacity_mah = 1500;    // the pack actually flown
    b.f.p.batt_capacity_mah = 2000;  // injected: setting left from the previous pack
    b.f.p.fence_radius = 300;
    for (int i = 0; i < 6; ++i) {
        b.mission.push_back({150, 0, 10});
        b.mission.push_back({150, 40, 10});
        b.mission.push_back({0, 40, 10});
        b.mission.push_back({0, 0, 10});
    }
    fly(b);

    std::printf("wrote log_a_flyaway.csv and log_b_late_battery.csv\n");
    return 0;
}
