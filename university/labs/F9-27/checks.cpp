// F9-27 checks: the worked example, the answers, and the damped fix for the forensic lab.
#include "jac.hpp"
#include <algorithm>
#include <cstdio>

int main()
{
    const double r = std::numbers::pi / 180;
    const V2 q{30 * r, 45 * r};
    const M2 J = jacobian(q);
    std::printf("J(30, 45) = [[%.5f, %.5f], [%.5f, %.5f]], det %.5f\n", J[0][0], J[0][1], J[1][0],
                J[1][1], det(J));
    const V2 v = mul(J, {10 * r, -20 * r});
    std::printf("qdot = (10, -20) deg/s gives tool velocity (%.5f, %.5f) m/s\n", v[0], v[1]);
    const V2 qd = solve(J, {0.0, 0.05});
    std::printf("tool velocity (0, 0.05) m/s needs qdot = (%.3f, %.3f) deg/s\n", deg(qd[0]),
                deg(qd[1]));
    const V2 tau = mul(transpose(J), {5.0, 0.0});
    std::printf("tau for F = (5, 0) N: (%.4f, %.4f) N m\n", tau[0], tau[1]);
    // answer: outstretched along x (t2 = 0): J columns
    const M2 S = jacobian({0.0, 0.0});
    std::printf("J(0, 0) = [[%.3f, %.3f], [%.3f, %.3f]], det %.3f\n", S[0][0] + 0.0, S[0][1] + 0.0,
                S[1][0], S[1][1], det(S));
    const V2 tz = mul(transpose(S), {-10.0, 0.0});
    std::printf("outstretched, push F = (-10, 0) N: tau = (%.3f, %.3f) N m\n", tz[0] + 0.0,
                tz[1] + 0.0);

    // forensic fix: the whip path with damped least squares (lambda = 0.02 m)
    for (double lambda : {0.0, 0.01, 0.02}) {
        const V2 start{0.30, 0.01}, goal{0.5495, 0.01};
        const double T = 4.0, dt = 0.01, kp = 2.0;
        const double c2 =
            (start[0] * start[0] + start[1] * start[1] - L1 * L1 - L2 * L2) / (2 * L1 * L2);
        V2 qq{0, std::acos(c2)};
        qq[0] = std::atan2(start[1], start[0]) -
                std::atan2(L2 * std::sin(qq[1]), L1 + L2 * std::cos(qq[1]));
        double peak = 0, endErr = 0;
        for (int k = 0; k <= 400; ++k) {
            const V2 want{start[0] + (goal[0] - start[0]) / T * k * dt, start[1]};
            const V2 p = fk(qq);
            const V2 vv{(goal[0] - start[0]) / T + kp * (want[0] - p[0]), kp * (want[1] - p[1])};
            const V2 d =
                lambda > 0 ? solveDamped(jacobian(qq), vv, lambda) : solve(jacobian(qq), vv);
            peak = std::max({peak, std::abs(deg(d[0])), std::abs(deg(d[1]))});
            endErr = std::hypot(want[0] - p[0], want[1] - p[1]);
            qq = {qq[0] + d[0] * dt, qq[1] + d[1] * dt};
        }
        std::printf("whip path, lambda = %.2f m: peak joint speed %8.2f deg/s, final tracking "
                    "error %.4f m\n",
                    lambda, peak, endErr);
    }
    return 0;
}
