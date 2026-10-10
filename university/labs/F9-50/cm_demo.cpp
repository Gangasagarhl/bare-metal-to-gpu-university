// cm_demo.cpp - F9-50 Listing 4: the uctl stack in simulated time. The loop runs 2,000
// cycles of 1 ms as fast as possible (no sleeping), so the output is the same on every run.
#include <cmath>
#include <cstdio>

#include "controllers.h"
#include "sim_hw.h"
#include "uctl.h"

int main()
{
    SimWheels hw;
    WheelVelocityPi pi;
    uctl::ControllerManager cm(hw);
    cm.addController(pi);
    cm.configure();                                   // all lookups and allocations happen here

    const auto st = hw.exportStates();                // views for printing, taken once
    const auto cmd = hw.exportCommands();
    const double dt = 0.001;                          // 1 kHz
    double maxErrEnd = 0;
    std::printf("   t(s)  target  left_vel right_vel  left_u right_u   (rad/s, effort -1..1)\n");
    for (int k = 0; k <= 2000; ++k) {
        const double t = k * dt;
        const double target = t < 0.1 ? 0.0 : (t < 1.0 ? 10.0 : 5.0);
        pi.setTarget(0, target);
        pi.setTarget(1, target);
        cm.cycle(t, dt);
        if (k % 100 == 0) {
            std::printf("  %5.2f  %6.2f  %8.3f %9.3f  %6.3f %7.3f\n", t, target, *st[1].value,
                        *st[3].value, *cmd[0].value, *cmd[1].value);
        }
        if (t > 1.9) {
            maxErrEnd = std::fmax(maxErrEnd, std::fmax(std::fabs(*st[1].value - target),
                                                       std::fabs(*st[3].value - target)));
        }
    }
    std::printf("largest speed error in the last 0.1 s: %.3f rad/s -> %s (tolerance 0.2)\n",
                maxErrEnd, maxErrEnd < 0.2 ? "PASS" : "FAIL");
    return 0;
}
