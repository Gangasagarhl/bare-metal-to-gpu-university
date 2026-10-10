// F9-28 checks: the worked example, the answers, and the step-size study behind the
// forensic answer key (same free swing, four integrators, energy after 10 s).
#include "dyn.hpp"
#include <cstdio>
#include <numbers>

double run(const Arm2& arm, int method, double h) // 0 Euler, 1 semi-implicit Euler, 2 RK4
{
    State s{{0, 0}, {0, 0}};
    const int n = static_cast<int>(10.0 / h + 0.5);
    for (int k = 0; k < n; ++k) {
        if (method == 2) {
            s = rk4(arm, s, {0, 0}, h);
            continue;
        }
        const V2 qdd = arm.forwardDyn(s.q, s.qd, {0, 0});
        if (method == 0) {
            s.q = {s.q[0] + h * s.qd[0], s.q[1] + h * s.qd[1]};
            s.qd = {s.qd[0] + h * qdd[0], s.qd[1] + h * qdd[1]};
        } else {
            s.qd = {s.qd[0] + h * qdd[0], s.qd[1] + h * qdd[1]};
            s.q = {s.q[0] + h * s.qd[0], s.q[1] + h * s.qd[1]};
        }
    }
    return arm.energy(s.q, s.qd);
}

int main()
{
    const Arm2 arm;
    const double r = std::numbers::pi / 180;
    const V2 g0 = arm.gravity({0, 0}), g90 = arm.gravity({90 * r, 0}),
             gm = arm.gravity({0, 90 * r});
    std::printf("g(0, 0)  = (%.5f, %.5f) N m\n", g0[0], g0[1]);
    std::printf("g(90, 0) = (%.5f, %.5f) N m\n", g90[0] + 0.0, g90[1] + 0.0);
    std::printf("g(0, 90) = (%.5f, %.5f) N m\n", gm[0], gm[1] + 0.0);
    const V2 a = arm.forwardDyn({0, 0}, {0, 0}, {0, 0});
    std::printf("released at (0, 0): qdd = (%.4f, %.4f) rad/s^2\n", a[0], a[1]);
    const V2 t = arm.inverseDyn({0, 0}, {0, 0}, {2.0, 0.0});
    std::printf("ID at (0,0), qd = 0, qdd = (2, 0): tau = (%.5f, %.5f) N m\n", t[0], t[1]);
    const V2 c = arm.coriolis({0, 90 * r}, {2.0, 0.0});
    std::printf("c at t2 = 90 deg, qd = (2, 0): (%.4f, %.4f) N m\n", c[0] + 0.0, c[1]);
    const char* names[] = {"explicit Euler     ", "semi-implicit Euler", "RK4                "};
    for (int m = 0; m < 3; ++m)
        for (double h : {0.005, 0.0005})
            std::printf("%s h = %.4f s: energy after 10 s = %10.2e J (start 0)\n", names[m], h,
                        run(arm, m, h));
    return 0;
}
