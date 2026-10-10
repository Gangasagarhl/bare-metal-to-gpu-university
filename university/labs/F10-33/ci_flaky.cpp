// ci_flaky.cpp - forensic evidence for F10-33: ten nights of the same SITL acceptance test
// on a shared continuous-integration machine. The test, the code and the seed of the
// simulated world never change. Generated with the course simulator; the host's load is
// modelled by a seeded scheduler (see lockstep.cpp), so this evidence is repeatable.
#include "dronesim.hpp"

#include <cstdio>
#include <vector>

struct Night
{
    const char* date;
    bool lockstep;
    double host_load;  // probability per step that the flight software is not scheduled
    std::uint64_t seed;
};

int main()
{
    const std::vector<Night> nights = {
        {"2026-09-01", false, 0.00, 11},  {"2026-09-02", false, 0.03, 100},
        {"2026-09-03", false, 0.00, 13},  {"2026-09-04", false, 0.03, 101},
        {"2026-09-05", false, 0.03, 116}, {"2026-09-06", false, 0.00, 16},
        {"2026-09-07", false, 0.03, 104}, {"2026-09-08", false, 0.03, 120},
        {"2026-09-09", false, 0.00, 19},  {"2026-09-10", true, 0.03, 106},
    };
    std::printf("SITL nightly: test 'square mission', world seed 42, commit a41f9c2 (unchanged)\n");
    std::printf("%-10s %-9s %-14s %8s %14s %8s\n", "night", "lockstep", "other CI jobs", "missed",
                "land err [m]", "A4<=1.2");
    for (const Night& n : nights) {
        dn::World w;
        w.wind = {1.0, 0.5, 0};
        dn::Fsw f;
        dn::Rng sched(n.seed);
        dn::Sensors s = w.sense();
        f.arm(s, {{20, 0, 10}, {20, 20, 10}, {0, 20, 10}});
        dn::Command c = f.step(s);
        int stall = 0, missed = 0;
        while (w.t < 300.0) {
            s = w.sense();
            if (n.lockstep) {
                c = f.step(s);
            } else if (stall > 0) {
                --stall;
                ++missed;
            } else {
                c = f.step(s);
                if (sched.uniform() < n.host_load)
                    stall = 5 + static_cast<int>(sched.uniform() * 40);
            }
            w.step(c);
            if (f.mode == dn::Mode::Disarmed || w.crashed) break;
        }
        const double err = dn::hnorm(w.pos - f.home);
        std::printf("%-10s %-9s %-14s %8d %14.3f %8s\n", n.date, n.lockstep ? "on" : "off",
                    n.host_load > 0 ? "yes" : "no", missed, err, err <= 1.2 ? "PASS" : "FAIL");
    }
    return 0;
}
