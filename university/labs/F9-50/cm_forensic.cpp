// cm_forensic.cpp - F9-50 forensic run: the same loop as cm_demo.cpp, built with the
// hardware interface of sim_hw_v2.h. It also prints each wheel's position, and the speed a
// monitoring tool computes from two positions 0.1 s apart.
#include <cmath>
#include <cstdio>

#include "controllers.h"
#include "sim_hw_v2.h"
#include "uctl.h"

int main()
{
    SimWheels hw;
    WheelVelocityPi pi;
    uctl::ControllerManager cm(hw);
    cm.addController(pi);
    cm.configure();

    const auto st = hw.exportStates();
    const auto cmd = hw.exportCommands();
    const double dt = 0.001;
    double lastPos[2] = {0, 0};
    std::printf("   t(s) target | left: vel_state  pos   speed_from_pos  u | "
                "right: vel_state  pos   speed_from_pos  u\n");
    for (int k = 0; k <= 1000; ++k) {
        const double t = k * dt;
        const double target = t < 0.1 ? 0.0 : 10.0;
        pi.setTarget(0, target);
        pi.setTarget(1, target);
        cm.cycle(t, dt);
        if (k % 100 == 0) {
            const double sl = (*st[0].value - lastPos[0]) / 0.1;
            const double sr = (*st[2].value - lastPos[1]) / 0.1;
            std::printf("  %5.2f %6.2f |  %8.3f %7.3f %9.3f %8.3f |  %8.3f %7.3f %9.3f %8.3f\n", t,
                        target, *st[1].value, *st[0].value, sl, *cmd[0].value, *st[3].value,
                        *st[2].value, sr, *cmd[1].value);
            lastPos[0] = *st[0].value;
            lastPos[1] = *st[2].value;
        }
    }
    return 0;
}
