// F9-29 forensic evidence generator: "The model says left, the arm says otherwise".
// The arm was moved to the left side of the base and the model file was edited
// (robot_v2.urdf). The simulator plays the real robot; its true mount is known only
// to this program and to the answer key.
#include "urdf_mini.hpp"
#include <cstdio>
#include <numbers>

int main()
{
    const Model model = Model::load("robot_v2.urdf");
    Model robot = model; // the simulated physical robot: same file, true mount below
    robot.byChild.at("arm_base").origin.R = rotAxis({0, 0, 1}, std::numbers::pi / 2);

    const JointModel& j = model.byChild.at("arm_base");
    const Tf home = model.pose("turret", {});
    std::printf("model check: %s, root %s, %zu links, one root: ok\n", model.name.c_str(),
                model.root.c_str(), model.mass.size());
    std::printf("joint %s (%s): xyz (%.3f, %.3f, %.3f), rpy (%.3f, %.3f, %.3f)\n", j.name.c_str(),
                j.type.c_str(), j.origin.t[0], j.origin.t[1], j.origin.t[2], j.rpy[0], j.rpy[1],
                j.rpy[2]);
    std::printf(
        "model: arm's forward direction in base_link at base_yaw = 0: (%.4f, %.4f, %.4f)\n\n",
        home.R[0][0], home.R[1][0], home.R[2][0]);

    const double d = std::numbers::pi / 180;
    const double tests[][3] = {{0, 0, 0}, {0, 30, -60}, {45, 20, -40}, {-90, 0, -90}, {90, 10, 0}};
    std::printf("base_yaw shoulder elbow (deg)   model: tool in base_link (m)   tracker (m)        "
                "      error (m)\n");
    for (const auto& t : tests) {
        const std::map<std::string, double> q{
            {"base_yaw", t[0] * d}, {"shoulder", t[1] * d}, {"elbow", t[2] * d}};
        const V3 a = model.pose("tool", q).t, b = robot.pose("tool", q).t;
        std::printf("%5.0f %8.0f %6.0f              (%7.4f, %7.4f, %7.4f)      (%7.4f, %7.4f, "
                    "%7.4f)  %.4f\n",
                    t[0], t[1], t[2], a[0], a[1], a[2], b[0], b[1], b[2],
                    std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]));
    }
    return 0;
}
