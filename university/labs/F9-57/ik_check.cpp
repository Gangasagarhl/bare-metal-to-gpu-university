// ik_check.cpp - F9-57 worked example: the pre-grasp IK solution step by step, then
// forward kinematics to confirm it, then the time of one trapezoidal joint move.
#include "arm.hpp"

int main()
{
    const double x = 0.62, z = 0.18, phi = -arm::kPi / 2;     // tool tip target, tool pointing down
    const double wx = x - arm::kL3 * std::cos(phi);
    const double wz = z - arm::kBaseZ - arm::kL3 * std::sin(phi);
    std::printf("wrist relative to joint 1: (%.4f, %.4f), distance %.4f m\n", wx, wz, std::hypot(wx, wz));
    const double c2 = (wx * wx + wz * wz - arm::kL1 * arm::kL1 - arm::kL2 * arm::kL2) / (2 * arm::kL1 * arm::kL2);
    std::printf("cos(q2) = %.4f\n", c2);
    for (int elbow : {-1, +1}) {
        const std::optional<arm::Q> q = arm::ik(x, z, phi, elbow);
        if (!q) { std::printf("elbow %+d: outside the joint limits\n", elbow); continue; }
        const arm::P2 t = arm::fk(*q)[3];
        std::printf("elbow %+d: q = (%.2f, %.2f, %.2f) deg; FK gives tip (%.4f, %.4f); collision: '%s'\n",
                    elbow, (*q)[0] * 180 / arm::kPi, (*q)[1] * 180 / arm::kPi, (*q)[2] * 180 / arm::kPi, t.x, t.z,
                    arm::Scene{}.collision(*q).c_str());
    }
    const arm::Q home{arm::kPi / 2, -arm::kPi / 2, -arm::kPi / 2};
    const arm::Q pre = *arm::ik(x, z, phi, -1);
    for (int j = 0; j < 3; ++j) { std::printf("joint %d moves %.4f rad from home\n", j + 1, std::fabs(pre[j] - home[j])); }
    std::printf("vmax^2 / amax = %.3f rad: a longer move reaches vmax\n", arm::kVMax * arm::kVMax / arm::kAMax);
    std::printf("home -> pre-grasp in one segment: %.3f s\n", arm::segmentTime(home, pre));
    std::printf("a 0.300 rad move: %.3f s (triangular profile)\n", arm::segmentTime({0, 0, 0}, {0.3, 0, 0}));
    return 0;
}
