// F9-28 forensic evidence generator: "The simulated arm swings higher and higher".
// A team's own simulator lets the arm swing freely (no torque, no friction) and logs
// its tool height and the arm's total energy. The defect is described in the answer key.
#include "dyn.hpp"
#include <cstdio>
#include <numbers>

int main()
{
    const Arm2 arm;
    const double h = 0.005; // simulator step (s)
    State s{{0, 0}, {0, 0}};
    std::printf("simulator step %.3f s, no torque, no friction\n", h);
    std::printf(" t (s)   q1 (deg)   q2 (deg)   tool y (m)   energy (J)\n");
    for (int k = 0; k <= 2000; ++k) {
        if (k % 200 == 0) {
            const double y = arm.L1 * std::sin(s.q[0]) + arm.L2 * std::sin(s.q[0] + s.q[1]);
            std::printf("%5.1f   %8.2f   %8.2f   %8.4f     %.4f\n", k * h,
                        s.q[0] * 180 / std::numbers::pi, s.q[1] * 180 / std::numbers::pi, y,
                        arm.energy(s.q, s.qd));
        }
        const V2 qdd = arm.forwardDyn(s.q, s.qd, {0, 0});
        s.q = {s.q[0] + h * s.qd[0], s.q[1] + h * s.qd[1]};
        s.qd = {s.qd[0] + h * qdd[0], s.qd[1] + h * qdd[1]};
    }
    return 0;
}
