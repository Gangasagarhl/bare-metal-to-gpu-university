// F9-28 Listing 2: three checks that the dynamics model is right before anyone uses it:
// inverse and forward dynamics agree, gravity compensation holds the arm still, and a
// freely swinging arm keeps its total energy.
#include "dyn.hpp"
#include <algorithm>
#include <cstdio>
#include <numbers>

int main()
{
    const Arm2 arm;
    const double r = std::numbers::pi / 180;

    // 1. round trip: tau = ID(q, qd, qdd), then FD(q, qd, tau) must give qdd back
    double worst = 0;
    for (int i = 0; i < 1000; ++i) {
        const V2 q{std::sin(1.3 * i) * 3, std::cos(0.7 * i) * 3},
            qd{std::sin(2.1 * i) * 4, std::cos(1.9 * i) * 4};
        const V2 qdd{std::sin(0.37 * i) * 20, std::cos(0.53 * i) * 20};
        const V2 back = arm.forwardDyn(q, qd, arm.inverseDyn(q, qd, qdd));
        worst = std::max({worst, std::abs(back[0] - qdd[0]), std::abs(back[1] - qdd[1])});
    }
    std::printf("1. ID then FD, 1000 random states: largest qdd difference %.2e rad/s^2\n", worst);

    // 2. gravity compensation at (30, 45) deg: tau = g(q) should hold the arm still
    State s{{30 * r, 45 * r}, {0, 0}};
    const V2 hold = arm.gravity(s.q);
    for (int k = 0; k < 2000; ++k) s = rk4(arm, s, hold, 0.001);
    std::printf("2. holding torques (%.4f, %.4f) N m; after 2 s the arm moved %.2e rad\n", hold[0],
                hold[1], std::hypot(s.q[0] - 30 * r, s.q[1] - 45 * r));

    // 3. free swing from horizontal and outstretched, no torque: energy must stay constant
    s = {{0, 0}, {0, 0}};
    const double e0 = arm.energy(s.q, s.qd);
    double drift = 0;
    std::printf("3. free swing, RK4 with h = 1 ms (start energy %.4f J)\n", e0);
    std::printf("   t (s)   q1 (deg)   q2 (deg)   energy change (J)\n");
    for (int k = 0; k <= 5000; ++k) {
        if (k % 1000 == 0) {
            std::printf("   %4.1f   %8.2f   %8.2f   %10.2e\n", k * 0.001, s.q[0] / r, s.q[1] / r,
                        arm.energy(s.q, s.qd) - e0);
        }
        drift = std::max(drift, std::abs(arm.energy(s.q, s.qd) - e0));
        s = rk4(arm, s, {0, 0}, 0.001);
    }
    std::printf("   largest energy change over 5 s: %.2e J\n", drift);

    // 4. the mass matrix depends on the pose
    for (double t2 : {0.0, 90.0, 180.0}) {
        const M2 M = arm.mass({0, t2 * r});
        std::printf("4. M at t2 = %5.1f deg: [[%.5f, %.5f], [%.5f, %.5f]] kg m^2\n", t2, M[0][0],
                    M[0][1], M[1][0], M[1][1]);
    }
    return 0;
}
