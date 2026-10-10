// F9-25 checks: the worked example and the answers, recomputed.
#include "arm.hpp"
#include <cstdio>

int main()
{
    // planar 2R worked example: L1 = 0.30, L2 = 0.25, theta1 = 30, theta2 = 45 degrees
    const double L1 = 0.30, L2 = 0.25, t1 = rad(30), t2 = rad(45);
    std::printf("elbow  (%.5f, %.5f)\n", L1 * std::cos(t1), L1 * std::sin(t1));
    std::printf("tip    (%.5f, %.5f)  tip angle %.1f deg\n",
                L1 * std::cos(t1) + L2 * std::cos(t1 + t2),
                L1 * std::sin(t1) + L2 * std::sin(t1 + t2), 75.0);
    // course arm at (q1, q2, q3) = (90, 30, -60) degrees
    const Transform T = courseArm().fk({rad(90), rad(30), rad(-60)});
    std::printf("course arm (90, 30, -60): tool (%.4f, %.4f, %.4f)\n", T.t[0], T.t[1], T.t[2]);
    std::printf("tool x axis in base: (%.4f, %.4f, %.4f)\n", T.R[0][0], T.R[1][0], T.R[2][0]);
    // answer: the course arm at (0, 90, -90)
    const Transform U = courseArm().fk({0, rad(90), rad(-90)});
    std::printf("course arm (0, 90, -90): tool (%.4f, %.4f, %.4f)\n", U.t[0], U.t[1], U.t[2]);
    // answer: farthest reach and the flipped-elbow prediction at (0, 0, 30)
    Arm flipped = courseArm();
    flipped.joints[2].axis = {0, 1, 0};
    const Transform F = flipped.fk({0, 0, rad(30)});
    std::printf("flipped elbow at (0, 0, 30): (%.4f, %.4f, %.4f); mirror of true z about 0.10\n",
                F.t[0], F.t[1], F.t[2]);
    return 0;
}
