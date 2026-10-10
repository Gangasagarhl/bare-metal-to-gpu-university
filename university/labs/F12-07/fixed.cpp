// fixed.cpp - the fix and its test: the controller compensates the reading's age, and the CI
// scenario matrix now sweeps latency up to the value measured on the robot (must pass) and one
// step beyond it (a margin probe: reported, not required).
#include "wallsim.hpp"

#include <cstdio>

int main()
{
    const double latencies[] = {0.0, 0.15, 0.3, 0.45, 0.6};
    std::printf("latency  old controller  compensating controller\n");
    int fails = 0;
    for (double lat : latencies) {
        wallsim::World w;
        w.start_d = 3.0;
        w.latency_s = lat;
        w.noise_m = 0.003;
        w.seed = 7;
        wallsim::Controller fixed;
        fixed.compensate = true;
        const auto a = wallsim::run(w, wallsim::Controller{}, [](const wallsim::Tick&) {});
        const auto b = wallsim::run(w, fixed, [](const wallsim::Tick&) {});
        const bool ok_b = wallsim::meets_requirements(b);
        if (lat <= 0.45 + 1e-9) {  // up to the latency measured on the robot: must pass
            fails += ok_b ? 0 : 1;
        }
        std::printf("%5.0f ms  %6.3f m %-5s  %6.3f m %s\n", lat * 1000, a.min_d,
                    wallsim::meets_requirements(a) ? "pass" : "FAIL", b.min_d,
                    ok_b ? "pass" : "FAIL");
    }
    std::printf("compensating controller, latency up to 450 ms: %d scenarios fail\n", fails);
    return fails == 0 ? 0 : 1;
}
