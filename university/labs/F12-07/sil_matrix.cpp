// sil_matrix.cpp - software-in-the-loop scenario matrix: the same controller in the simulated
// world, swept over sensor latency, noise and start distance. Prints the closest approach.
#include "wallsim.hpp"

#include <cstdio>

int main()
{
    const double latencies[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5};
    const double noises[] = {0.0, 0.02};
    const double starts[] = {2.0, 4.0};
    std::printf("latency  noise  start   min distance  result\n");
    int fails = 0;
    for (double lat : latencies) {
        for (double noise : noises) {
            for (double start : starts) {
                wallsim::World w;
                w.latency_s = lat;
                w.noise_m = noise;
                w.start_d = start;
                const auto o = wallsim::run(w, wallsim::Controller{}, [](const wallsim::Tick&) {});
                const bool ok = wallsim::meets_requirements(o);
                fails += ok ? 0 : 1;
                std::printf("%5.0f ms %4.0f mm %4.1f m   %8.3f m    %s\n", lat * 1000, noise * 1000,
                            start, o.min_d, ok ? "pass" : "FAIL");
            }
        }
    }
    std::printf("%d of 24 scenarios fail\n", fails);
    return 0;
}
