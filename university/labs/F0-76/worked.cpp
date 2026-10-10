// F0-76 worked example check: one step of h = 0.1 s from theta = 1 rad, omega = 0, three ways.
#include <cmath>
#include <cstdio>

#include "pendulum.hpp"

int main()
{
    const State s{1.0, 0.0};
    const double h = 0.1;
    const State k1 = deriv(s);
    const State k2 = deriv({s.theta + 0.5 * h * k1.theta, s.omega + 0.5 * h * k1.omega});
    const State k3 = deriv({s.theta + 0.5 * h * k2.theta, s.omega + 0.5 * h * k2.omega});
    const State k4 = deriv({s.theta + h * k3.theta, s.omega + h * k3.omega});
    std::printf("k1 = (%.6f, %.6f)\nk2 = (%.6f, %.6f)\nk3 = (%.6f, %.6f)\nk4 = (%.6f, %.6f)\n",
                k1.theta, k1.omega, k2.theta, k2.omega, k3.theta, k3.omega, k4.theta, k4.omega);
    const State e = eulerStep(s, h), si = semiImplicitStep(s, h), r = rk4Step(s, h);
    State ref = s;
    for (int i = 0; i < 10000; ++i) {
        ref = rk4Step(ref, h / 10000.0);
    }
    std::printf("Euler      theta = %.6f  omega = %.6f\n", e.theta, e.omega);
    std::printf("semi-impl. theta = %.6f  omega = %.6f\n", si.theta, si.omega);
    std::printf("RK4        theta = %.6f  omega = %.6f\n", r.theta, r.omega);
    std::printf("reference  theta = %.6f  omega = %.6f  (RK4, 10000 sub-steps)\n", ref.theta,
                ref.omega);
    return 0;
}
