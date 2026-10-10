// F9-25 Listing 2: forward kinematics of the course arm, checked against an independent
// closed-form (geometric) formula at many joint angles.
#include "arm.hpp"
#include <algorithm>
#include <cstdio>

// independent check: geometry by hand. Reach in the arm's vertical plane, then turn by q1.
Vec3 closedForm(double q1, double q2, double q3)
{
    const double L1 = 0.30, L2 = 0.30, h0 = 0.10; // L2 = forearm + tool
    const double reach = L1 * std::cos(q2) + L2 * std::cos(q2 + q3);
    const double z = h0 + L1 * std::sin(q2) + L2 * std::sin(q2 + q3);
    return {reach * std::cos(q1), reach * std::sin(q1), z};
}

int main()
{
    const Arm arm = courseArm();
    const double poses[][3] = {{0, 0, 0}, {90, 0, 0}, {0, 90, 0}, {0, 30, -60}, {45, 30, -60}};
    std::printf("  q1    q2    q3  (deg)   tool position from the chain (m)\n");
    for (const auto& p : poses) {
        const Transform T = arm.fk({rad(p[0]), rad(p[1]), rad(p[2])});
        std::printf("%4.0f  %4.0f  %4.0f          (%7.4f, %7.4f, %7.4f)\n", p[0], p[1], p[2],
                    T.t[0], T.t[1], T.t[2]);
    }
    // sweep: 9261 joint configurations, largest disagreement between the two methods
    double worst = 0;
    int count = 0;
    for (int a = -180; a <= 180; a += 18)
        for (int b = -90; b <= 90; b += 9)
            for (int c = -150; c <= 150; c += 15) {
                const Transform T = arm.fk({rad(a), rad(b), rad(c)});
                const Vec3 g = closedForm(rad(a), rad(b), rad(c));
                for (int i = 0; i < 3; ++i) worst = std::max(worst, std::abs(T.t[i] - g[i]));
                ++count;
            }
    std::printf("checked %d configurations; largest difference chain vs closed form: %.3e m\n",
                count, worst);
    return worst < 1e-12 ? 0 : 1;
}
