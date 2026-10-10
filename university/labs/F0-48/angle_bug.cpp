// F0-48 forensic evidence: the robot's bearing log. Contains ONE deliberate mistake.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec2
{
    double x;
    double y;
};

double dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

double length(Vec2 a)
{
    return std::sqrt(dot(a, a));
}

double angleDegrees(Vec2 forward, Vec2 to)
{
    const double c = dot(forward, to) / length(forward);
    return std::acos(c) * 180.0 / std::numbers::pi;
}

int main()
{
    const Vec2 forward{0.6, 0.8};  // the robot faces north-east; length 1
    const Vec2 targets[] = {{0.6, 0.8}, {0.0, 1.0}, {0.8, -0.6}, {3.0, 4.0}, {0.0, 2.0}, {-1.2, -1.6}};
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "forward = (0.600, 0.800)\n";
    std::cout << "   target.x   target.y   angle (deg)\n";
    for (const Vec2& t : targets) {
        std::cout << std::setw(11) << t.x << std::setw(11) << t.y << std::setw(14)
                  << angleDegrees(forward, t) << "\n";
    }
    return 0;
}
