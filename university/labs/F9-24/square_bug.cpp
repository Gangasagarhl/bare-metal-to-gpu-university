// F9-24 forensic evidence generator: "The square that does not close".
// The robot drives a 1 m square: straight 1 m, turn 90 degrees in place, four times.
// Two records are printed: the robot's own odometry, and an overhead tracker
// (simulator ground truth). The fault is described only in the answer key.
#include "diffdrive.hpp"
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double r = 0.05;
    const double W_config = 0.30; // value in the robot's configuration file
    const double W_robot = 0.34;  // the simulated robot's real wheel spacing
    const double h = 1e-4;
    Pose odom, truth;
    double t = 0;

    auto drive = [&](Twist2 cmd, double seconds) {
        const Wheels q = inverseKin(cmd, r, W_config); // controller converts command to wheels
        const int n = static_cast<int>(std::lround(seconds / h));
        for (int k = 0; k < n; ++k) {
            truth = stepArc(truth, forwardKin(q, r, W_robot), h); // what the robot really does
            odom = stepArc(odom, forwardKin(q, r, W_config), h);  // what odometry believes
            t += h;
        }
    };
    auto deg = [](double a) { return a * 180.0 / std::numbers::pi; };

    std::printf("   t(s)  segment        odometry x, y, heading        tracker x, y, heading\n");
    for (int side = 1; side <= 4; ++side) {
        drive({0.2, 0.0}, 5.0); // 1 m straight at 0.2 m/s
        std::printf("%7.2f  straight %d   (%7.3f, %7.3f, %7.1f deg)   (%7.3f, %7.3f, %7.1f deg)\n",
                    t, side, odom.x, odom.y, deg(odom.theta), truth.x, truth.y, deg(truth.theta));
        drive({0.0, 0.5}, (std::numbers::pi / 2) / 0.5); // 90 degrees at 0.5 rad/s
        std::printf("%7.2f  turn %d       (%7.3f, %7.3f, %7.1f deg)   (%7.3f, %7.3f, %7.1f deg)\n",
                    t, side, odom.x, odom.y, deg(odom.theta), truth.x, truth.y, deg(truth.theta));
    }
    std::printf("gap between start and end of the square (tracker): %.3f m\n",
                std::hypot(truth.x, truth.y));

    odom = truth = Pose{};
    drive({0.0, 0.5}, (2 * std::numbers::pi) / 0.5); // one full turn on the spot
    std::printf("spin test: commanded 360.0 deg, odometry %.1f deg, tracker %.1f deg\n",
                deg(odom.theta), deg(truth.theta));
    odom = truth = Pose{};
    drive({0.2, 0.0}, 10.0); // 2 m straight
    std::printf("straight test: commanded 2.000 m, odometry %.3f m, tracker %.3f m\n", odom.x,
                truth.x);
    return 0;
}
