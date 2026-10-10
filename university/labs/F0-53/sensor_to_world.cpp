// F0-53 Listing 1: a simulated robot's range readings, turned from the robot frame into the world frame.
// The room is the rectangle 0 <= x <= 6, 0 <= y <= 4 (metres) in the world frame W.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>

struct Vec2
{
    double x;
    double y;
};

struct Pose  // where the robot frame B sits in the world frame W
{
    double x;      // position of B's origin, in W (m)
    double y;
    double theta;  // heading: angle from W's x axis to B's x axis (rad, anticlockwise)
};

double radians(double degrees)
{
    return degrees * std::numbers::pi / 180.0;
}

// p_W = R(theta) p_B + t   (robot frame -> world frame)
Vec2 robotToWorld(const Pose& pose, Vec2 pB)
{
    const double c = std::cos(pose.theta), s = std::sin(pose.theta);
    return {c * pB.x - s * pB.y + pose.x, s * pB.x + c * pB.y + pose.y};
}

// The simulator: distance from the robot along a beam until it hits a wall.
double simulateRange(const Pose& pose, double bearing)
{
    const double a = pose.theta + bearing;  // beam direction in W
    const double dx = std::cos(a), dy = std::sin(a);
    double best = std::numeric_limits<double>::infinity();
    if (dx > 1e-12) best = std::min(best, (6.0 - pose.x) / dx);
    if (dx < -1e-12) best = std::min(best, (0.0 - pose.x) / dx);
    if (dy > 1e-12) best = std::min(best, (4.0 - pose.y) / dy);
    if (dy < -1e-12) best = std::min(best, (0.0 - pose.y) / dy);
    return best;
}

double tidy(double v)
{
    return std::abs(v) < 5e-10 ? 0.0 : v;  // print -0.000 as 0.000
}

double distanceToNearestWall(Vec2 pW)
{
    return std::min({std::abs(pW.x), std::abs(6.0 - pW.x), std::abs(pW.y), std::abs(4.0 - pW.y)});
}

int main()
{
    const Pose poses[] = {{2.0, 1.0, radians(0.0)}, {2.0, 1.0, radians(30.0)}, {4.5, 3.0, radians(-120.0)}};
    const double bearingsDeg[] = {-60.0, 0.0, 45.0, 90.0};
    std::cout << std::fixed << std::setprecision(3);
    double worst = 0.0;
    for (const Pose& pose : poses) {
        std::cout << "robot at (" << pose.x << ", " << pose.y << "), heading "
                  << pose.theta * 180.0 / std::numbers::pi << " deg\n";
        std::cout << "  bearing   range    point in robot frame    point in world frame   off wall\n";
        for (double b : bearingsDeg) {
            const double r = simulateRange(pose, radians(b));
            const Vec2 pB{r * std::cos(radians(b)), r * std::sin(radians(b))};
            const Vec2 pW = robotToWorld(pose, pB);
            const double off = distanceToNearestWall(pW);
            worst = std::max(worst, off);
            std::cout << std::setw(9) << b << std::setw(8) << r << "   (" << std::setw(7) << pB.x << ", "
                      << std::setw(7) << pB.y << ")    (" << std::setw(6) << tidy(pW.x) << ", " << std::setw(6)
                      << tidy(pW.y) << ")   " << std::scientific << std::setprecision(1) << off << std::fixed
                      << std::setprecision(3) << "\n";
        }
    }
    std::cout << "largest distance of any world point from a wall: " << std::scientific << worst << "\n";
    return worst < 1e-9 ? 0 : 1;
}
