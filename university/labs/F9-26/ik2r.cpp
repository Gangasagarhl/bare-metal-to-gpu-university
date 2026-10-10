// F9-26 Listing 2: both IK branches for several targets, each checked by running
// forward kinematics on the answer (the round-trip test).
#include "ik.hpp"
#include <cstdio>

int main()
{
    const double L1 = 0.30, L2 = 0.25;
    const double targets[][2] = {{0.40, 0.20}, {0.30, 0.25}, {0.55, 0.00}, {0.00, 0.55},
                                 {0.60, 0.00}, {0.03, 0.00}, {0.05, 0.00}};
    std::printf("target (m)        branch      t1 (deg)   t2 (deg)   FK round-trip error (m)\n");
    for (const auto& p : targets) {
        for (bool up : {true, false}) {
            const auto a = ik2r(L1, L2, p[0], p[1], up);
            std::printf("(%5.2f, %5.2f)    %-10s  ", p[0], p[1], up ? "elbow-up" : "elbow-down");
            if (!a) {
                std::printf("out of reach (needs %.2f <= distance <= %.2f, has %.4f)\n",
                            std::abs(L1 - L2), L1 + L2, std::hypot(p[0], p[1]));
                continue;
            }
            double x = 0, y = 0;
            fk2r(L1, L2, *a, x, y);
            std::printf("%8.3f   %8.3f   %.2e\n", deg(a->t1), deg(a->t2),
                        std::hypot(x - p[0], y - p[1]));
        }
    }
    return 0;
}
