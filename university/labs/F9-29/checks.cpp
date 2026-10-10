// F9-29 checks: numbers used in the worked example and the answer key.
#include "urdf_mini.hpp"
#include <cstdio>
#include <numbers>

int main()
{
    const double pi = std::numbers::pi;
    const double wrapped =
        std::remainder(90.0, 2 * pi); // 90 taken as radians, wrapped to (-pi, pi]
    std::printf("90 rad = %.4f turns; wrapped to %.4f rad = %.2f deg\n", 90.0 / (2 * pi), wrapped,
                wrapped * 180 / pi);
    std::printf("error angle against the intended 90 deg: %.2f deg\n", wrapped * 180 / pi - 90.0);
    // worked example: tool of the course robot at base_yaw 90, shoulder 30, elbow -60 deg
    const Model m = Model::load("course_robot.urdf");
    const double d = pi / 180;
    const Tf T = m.pose("tool", {{"base_yaw", 90 * d}, {"shoulder", 30 * d}, {"elbow", -60 * d}});
    std::printf("tool at (90, 30, -60): (%.4f, %.4f, %.4f) in base_link\n", T.t[0] + 0.0, T.t[1],
                T.t[2]);
    // rpy (0, 0, pi/2) applied to x axis
    const Tf Y{rotAxis({0, 0, 1}, pi / 2), {}};
    std::printf("yaw 90 deg turns x axis to (%.4f, %.4f, %.4f)\n", Y.R[0][0] + 0.0, Y.R[1][0],
                Y.R[2][0]);
    // rpy (pi/2, 0, pi/2): Rz Ry Rx applied to the z axis
    const Tf Z = Tf{rotAxis({0, 0, 1}, pi / 2), {}} * Tf{rotAxis({0, 1, 0}, 0), {}} *
                 Tf{rotAxis({1, 0, 0}, pi / 2), {}};
    std::printf("rpy (90, 0, 90 deg): child z axis in parent = (%.4f, %.4f, %.4f)\n", Z.R[0][2],
                std::abs(Z.R[1][2]) < 1e-12 ? 0.0 : Z.R[1][2],
                std::abs(Z.R[2][2]) < 1e-12 ? 0.0 : Z.R[2][2]);
    return 0;
}
