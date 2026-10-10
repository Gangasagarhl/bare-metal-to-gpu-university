// F0-48 Listing 1: lengths, dot products and angles for a robot that looks for its charger.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec2
{
    double x;
    double y;
};

Vec2 subtract(Vec2 a, Vec2 b)
{
    return {a.x - b.x, a.y - b.y};
}

double dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

double length(Vec2 a)
{
    return std::sqrt(dot(a, a));
}

Vec2 normalised(Vec2 a)
{
    const double len = length(a);
    return {a.x / len, a.y / len};
}

double angleDegrees(Vec2 a, Vec2 b)
{
    double c = dot(a, b) / (length(a) * length(b));
    c = std::clamp(c, -1.0, 1.0);  // rounding can push c a hair outside [-1, 1]
    return std::acos(c) * 180.0 / std::numbers::pi;
}

void report(const char* name, Vec2 robot, Vec2 forward, Vec2 thing, double cosHalfView)
{
    const Vec2 to = subtract(thing, robot);
    const double ahead = dot(forward, to);  // forward has length 1
    const double c = ahead / length(to);
    std::cout << std::setw(8) << name << std::setw(10) << length(to) << std::setw(10) << ahead
              << std::setw(10) << angleDegrees(forward, to) << "   "
              << (c >= cosHalfView ? "in view" : "not in view") << "\n";
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    const Vec2 robot{1.0, 1.0};
    const Vec2 forward{1.0, 0.0};  // the robot faces east
    const Vec2 to{3.0, 4.0};
    std::cout << "length of (3, 4): " << length(to) << "\n";
    const Vec2 u = normalised(to);
    std::cout << "unit vector along (3, 4): (" << u.x << ", " << u.y << "), length " << length(u)
              << "\n";
    const double cosHalfView = std::cos(60.0 * std::numbers::pi / 180.0);  // camera sees +-60 deg
    std::cout << "cos(60 deg) = " << cosHalfView << "\n";
    std::cout << "   thing  distance     ahead     angle   camera\n";
    report("charger", robot, forward, {4.0, 5.0}, cosHalfView);
    report("sofa", robot, forward, {-1.0, 3.0}, cosHalfView);
    report("door", robot, forward, {1.0, 4.0}, cosHalfView);
    report("bowl", robot, forward, {3.0, 0.0}, cosHalfView);
    return 0;
}
