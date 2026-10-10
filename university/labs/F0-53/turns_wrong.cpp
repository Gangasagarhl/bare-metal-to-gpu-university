// F0-53 forensic evidence: a robot drives to a goal; this is its pose log.
// The program contains ONE deliberate mistake (see the chapter's forensic lab).
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec2
{
    double x;
    double y;
};

struct Pose
{
    double x;
    double y;
    double theta;
};

// Express a world point in the robot frame: p_B = R(theta)^T (p_W - t)
Vec2 worldToRobot(const Pose& pose, Vec2 pW)
{
    const double c = std::cos(pose.theta), s = std::sin(pose.theta);
    const double dx = pW.x - pose.x, dy = pW.y - pose.y;
    return {c * dx - s * dy, s * dx + c * dy};
}

int main()
{
    const Vec2 goal{4.0, 3.0};
    Pose pose{0.0, 0.0, 0.0};
    const double dt = 0.5, speed = 0.4, gain = 1.0, maxTurn = 0.8;  // s, m/s, 1/s, rad/s
    const double toDeg = 180.0 / std::numbers::pi;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "goal in world frame: (4.00, 3.00)\n";
    std::cout << "    t      x      y   heading   bearing used   turn  rate\n";
    for (int step = 0; step <= 12; ++step) {
        const Vec2 g = worldToRobot(pose, goal);
        const double bearing = std::atan2(g.y, g.x);
        double turn = gain * bearing;
        turn = std::max(-maxTurn, std::min(maxTurn, turn));
        std::cout << std::setw(5) << step * dt << std::setw(7) << pose.x << std::setw(7) << pose.y
                  << std::setw(10) << pose.theta * toDeg << std::setw(15) << bearing * toDeg
                  << std::setw(16) << (turn > 0 ? "left" : "right") << std::setw(8) << turn * toDeg
                  << " deg/s\n";
        pose.theta += turn * dt;
        pose.x += speed * dt * std::cos(pose.theta);
        pose.y += speed * dt * std::sin(pose.theta);
    }
    return 0;
}
