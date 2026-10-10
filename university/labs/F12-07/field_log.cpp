// field_log.cpp - forensic evidence 2: a stand-in for the robot's own log from the warehouse.
// There is no robot in this build: this log is produced by the same simulator with one
// property of the world changed (the answer key says which). Columns are what the robot logs:
// receive time, the sensor's capture stamp, the reading, the command, the wheel-encoder speed.
#include "wallsim.hpp"

#include <cstdio>

int main()
{
    wallsim::World w;
    w.start_d = 3.0;
    w.latency_s = 0.45;
    w.noise_m = 0.003;
    w.seed = 7;
    std::printf("  t_rx[s]  stamp[s]  range[mm]  cmd[mm/s]  wheel[mm/s]\n");
    int k = 0;
    bool stopped_logged = false;
    const auto o = wallsim::run(w, wallsim::Controller{}, [&](const wallsim::Tick& t) {
        const bool moving = t.speed > 0.0005 || t.cmd > 0.0;
        if ((k++ % 2 == 0 && moving) || (!moving && t.t > 0.5 && !stopped_logged)) {
            std::printf("%8.2f  %8.2f  %9.0f  %9.0f  %11.0f\n", t.t, t.stamp, t.reading * 1000,
                        t.cmd * 1000, t.speed * 1000);
            stopped_logged = !moving && t.t > 0.5;
        }
    });
    std::printf("after the stop, tape measure from sensor to shelf: %.0f mm\n", o.final_d * 1000);
    return 0;
}
