// ci_sim.cpp - forensic evidence 1: the simulation test exactly as the team's CI ran it.
#include "wallsim.hpp"

#include <cstdio>

int main()
{
    wallsim::World w;  // the CI configuration: every field at its default
    w.start_d = 3.0;
    std::printf("ci scenario 'approach_shelf': start %.2f m, sensor latency %.0f ms, "
                "noise %.0f mm, accel %.1f m/s^2, seed %u\n",
                w.start_d, w.latency_s * 1000, w.noise_m * 1000, w.accel, w.seed);
    const auto o = wallsim::run(w, wallsim::Controller{}, [](const wallsim::Tick&) {});
    std::printf("closest approach %.3f m, final distance %.3f m, final speed %.3f m/s\n", o.min_d,
                o.final_d, o.final_v);
    const bool ok = wallsim::meets_requirements(o);
    std::printf("requirement S1 (>= 0.35 m) and S2 (stopped within 0.65 m): %s\n",
                ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
