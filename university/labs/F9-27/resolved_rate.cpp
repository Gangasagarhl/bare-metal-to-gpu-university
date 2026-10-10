// F9-27 Listing 3: resolved-rate control. Move the tool along a straight line at a
// constant speed by converting the wanted tool velocity into joint velocities every
// 10 ms (qdot = J^-1 v), with a small feedback term that corrects drift.
#include "jac.hpp"
#include <cstdio>

int main()
{
    const V2 start{0.30, 0.20}, goal{0.45, -0.05};
    // duration (s), control period (s), drift-correction gain (1/s)
    const double T = 3.0, dt = 0.01, kp = 2.0;
    const V2 vLine{(goal[0] - start[0]) / T, (goal[1] - start[1]) / T};

    // start pose from analytic IK (F9-26), elbow-down branch
    const double d2 = start[0] * start[0] + start[1] * start[1];
    const double c2 = (d2 - L1 * L1 - L2 * L2) / (2 * L1 * L2);
    V2 q{0, std::acos(c2)};
    q[0] =
        std::atan2(start[1], start[0]) - std::atan2(L2 * std::sin(q[1]), L1 + L2 * std::cos(q[1]));

    std::printf(" t (s)   wanted (m)          actual (m)          qdot (deg/s)          det J\n");
    const int steps = static_cast<int>(T / dt + 0.5);
    for (int k = 0; k <= steps; ++k) {
        const double t = k * dt;
        const V2 want{start[0] + vLine[0] * t, start[1] + vLine[1] * t};
        const V2 p = fk(q);
        const V2 v{vLine[0] + kp * (want[0] - p[0]), vLine[1] + kp * (want[1] - p[1])};
        const M2 J = jacobian(q);
        const V2 qd = solve(J, v);
        if (k % 50 == 0) {
            std::printf("%5.2f   (%6.4f, %7.4f)   (%6.4f, %7.4f)   (%8.3f, %8.3f)   %.5f\n", t,
                        want[0], want[1], p[0], p[1], deg(qd[0]), deg(qd[1]), det(J));
        }
        q = {q[0] + qd[0] * dt, q[1] + qd[1] * dt}; // the simulated joints follow exactly
    }
    return 0;
}
