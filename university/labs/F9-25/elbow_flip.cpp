// F9-25 forensic evidence generator: "Right at home, wrong everywhere else".
// The simulated arm reports where its tool really is (a measurement by an external
// tracker, here the simulator). The controller predicts the tool with its model.
// The model error is described only in the answer key.
#include "arm.hpp"
#include <cstdio>

int main()
{
    const Arm robot = courseArm(); // the physical (simulated) arm
    Arm model = courseArm();       // the controller's model, as configured
    model.joints[2].axis = {0, 1, 0};

    const double tests[][3] = {{0, 0, 0},   {30, 0, 0},  {0, 40, 0},   {0, 0, 30},
                               {0, 0, -30}, {0, 40, 30}, {60, 20, 45}, {-45, 10, -90}};
    std::printf("  q1   q2   q3 (deg)   model predicts (m)          tracker measures (m)        "
                "error (m)\n");
    for (const auto& d : tests) {
        const std::vector<double> q{rad(d[0]), rad(d[1]), rad(d[2])};
        const Vec3 a = model.fk(q).t, b = robot.fk(q).t;
        const double e = std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
        std::printf(
            "%4.0f %4.0f %4.0f       (%7.4f, %7.4f, %7.4f)    (%7.4f, %7.4f, %7.4f)    %.4f\n",
            d[0], d[1], d[2], a[0], a[1], a[2], b[0], b[1], b[2], e);
    }
    return 0;
}
