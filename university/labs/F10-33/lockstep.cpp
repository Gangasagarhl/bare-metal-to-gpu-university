// lockstep.cpp - why SITL simulators run the flight software in lockstep.
// Lockstep: the simulator advances one step only after the flight software has answered.
// Free-running: the simulator follows the wall clock; when the host is busy, the flight
// software misses steps, so the vehicle keeps flying on a stale command. The host's
// "busyness" is MODELLED here with a seeded random scheduler, so the run is repeatable;
// on a real machine it comes from the operating system and is not repeatable at all.
#include "dronesim.hpp"

#include <cstdio>
#include <vector>

struct Outcome
{
    double land_err, max_alt, t_end;
    int missed;
};

Outcome fly(bool lockstep, std::uint64_t sched_seed, double p_stall)
{
    dn::World w;
    w.wind = {1.0, 0.5, 0};
    dn::Fsw f;
    const std::vector<dn::V3> square = {{20, 0, 10}, {20, 20, 10}, {0, 20, 10}};
    dn::Rng sched(sched_seed);  // stands in for the host's scheduler
    dn::Sensors s = w.sense();
    f.arm(s, square);
    dn::Command c = f.step(s);
    int stall = 0, missed = 0;
    double max_alt = 0;
    while (w.t < 300.0) {
        s = w.sense();
        if (lockstep) {
            c = f.step(s);  // the world waits for the answer, always
        } else if (stall > 0) {
            --stall;
            ++missed;  // software not scheduled: the world keeps the old command
        } else {
            c = f.step(s);
            if (sched.uniform() < p_stall) stall = 5 + static_cast<int>(sched.uniform() * 40);
        }
        w.step(c);
        max_alt = std::max(max_alt, w.pos.z);
        if (f.mode == dn::Mode::Disarmed || w.crashed) break;
    }
    return {dn::hnorm(w.pos - f.home), max_alt, w.t, missed};
}

int main()
{
    std::printf("%-12s %5s %7s %13s %12s %10s\n", "mode", "seed", "missed", "land err [m]",
                "max alt [m]", "t_end [s]");
    for (bool lock : {true, false}) {
        for (std::uint64_t seed : {1u, 2u, 3u, 4u}) {
            const Outcome o = fly(lock, seed, 0.03);
            std::printf("%-12s %5llu %7d %13.3f %12.3f %10.2f\n",
                        lock ? "lockstep" : "free-running", static_cast<unsigned long long>(seed),
                        o.missed, o.land_err, o.max_alt, o.t_end);
        }
    }
    return 0;
}
