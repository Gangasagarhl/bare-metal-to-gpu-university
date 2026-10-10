// F9-26 forensic evidence generator: "The arm that freezes at full stretch".
// A sweep along the edge of the table at the arm's full reach (0.55 m), plus a few
// points just inside. This is the IK function the robot was running (an older
// version than Listing 1). The defect is described only in the answer key.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

const double L1 = 0.30, L2 = 0.25;

bool ikOld(double x, double y, double& t1, double& t2)
{
    const double c2 = (x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2);
    if (std::hypot(x, y) > L1 + L2) return false; // out of reach
    t2 = std::acos(c2);                           // elbow-down branch
    t1 = std::atan2(y, x) - std::atan2(L2 * std::sin(t2), L1 + L2 * std::cos(t2));
    return true;
}

int main()
{
    std::printf("step  target angle  radius (m)   x (m)     y (m)     t1 (deg)    t2 (deg)\n");
    int step = 0;
    for (double radius : {0.54, 0.55}) {
        for (int a = -40; a <= 40; a += 10) {
            const double phi = a * std::numbers::pi / 180.0;
            const double x = radius * std::cos(phi), y = radius * std::sin(phi);
            double t1 = 0, t2 = 0;
            const bool ok = ikOld(x, y, t1, t2);
            std::printf("%3d   %6d       %6.2f     %7.4f  %8.4f   ", ++step, a, radius, x, y);
            if (!ok) {
                std::printf("refused: out of reach\n");
            } else {
                std::printf("%9.3f   %9.3f\n", t1 * 180 / std::numbers::pi,
                            t2 * 180 / std::numbers::pi);
            }
        }
    }
    return 0;
}
