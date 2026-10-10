// F9-27 forensic evidence generator: "The arm whips at the end of a reach".
// Same resolved-rate controller as Listing 3, asked to slide the tool outwards along
// y = 0.01 m to x = 0.5495 m. The simulated joints are rated 120 deg/s.
#include "jac.hpp"
#include <cstdio>

int main()
{
    const V2 start{0.30, 0.01}, goal{0.5495, 0.01};
    const double T = 4.0, dt = 0.01, kp = 2.0, limit = 120.0;
    const V2 vLine{(goal[0] - start[0]) / T, 0.0};
    const double c2 =
        (start[0] * start[0] + start[1] * start[1] - L1 * L1 - L2 * L2) / (2 * L1 * L2);
    V2 q{0, std::acos(c2)};
    q[0] =
        std::atan2(start[1], start[0]) - std::atan2(L2 * std::sin(q[1]), L1 + L2 * std::cos(q[1]));

    std::printf(
        " t (s)   tool x (m)  tool speed (m/s)  t2 (deg)   qdot1 (deg/s)  qdot2 (deg/s)  note\n");
    const int steps = static_cast<int>(T / dt + 0.5);
    for (int k = 0; k <= steps; ++k) {
        const double t = k * dt;
        const V2 want{start[0] + vLine[0] * t, start[1]};
        const V2 p = fk(q);
        const V2 v{vLine[0] + kp * (want[0] - p[0]), vLine[1] + kp * (want[1] - p[1])};
        const V2 qd = solve(jacobian(q), v);
        const bool over = std::abs(deg(qd[0])) > limit || std::abs(deg(qd[1])) > limit;
        if (k % 40 == 0 || (k > 360 && k % 4 == 0)) {
            std::printf("%5.2f   %8.4f    %8.4f          %7.3f   %10.2f     %10.2f     %s\n", t,
                        p[0], std::hypot(v[0], v[1]), deg(q[1]), deg(qd[0]), deg(qd[1]),
                        over ? "OVER JOINT LIMIT" : "");
        }
        q = {q[0] + qd[0] * dt, q[1] + qd[1] * dt};
    }
    return 0;
}
