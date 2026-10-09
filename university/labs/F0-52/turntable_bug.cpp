// F0-52 forensic evidence: a turntable controller logs where a marker ends up. Contains ONE deliberate mistake.
#include <cmath>
#include <iomanip>
#include <iostream>

struct Vec2
{
    double x;
    double y;
};

Vec2 rotate(Vec2 p, double angle)  // angle as typed by the user, in degrees
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {c * p.x - s * p.y, s * p.x + c * p.y};
}

double tidy(double x)
{
    return std::abs(x) < 5e-10 ? 0.0 : x;
}

int main()
{
    const Vec2 marker{1.0, 0.0};
    const double commands[] = {0.0, 30.0, 90.0, 180.0, 360.0};
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "command (deg)   marker.x   marker.y   distance from centre\n";
    for (double cmd : commands) {
        const Vec2 m = rotate(marker, cmd);
        std::cout << std::setw(13) << cmd << std::setw(11) << tidy(m.x) << std::setw(11) << tidy(m.y)
                  << std::setw(14) << std::hypot(m.x, m.y) << "\n";
    }
    return 0;
}
