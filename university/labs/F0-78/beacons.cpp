// F0-78 forensic evidence: a robot finds its position from the bearings to two beacons.
// Log of 5 fixes taken while the robot stands still at two places in the hall.
// Each bearing has a small error of +/-0.2 degrees (the error this scenario assumes for the
// sensor).
#include <cmath>
#include <cstdio>
#include <numbers>

struct P
{
    double x, y;
};

// Intersect the line through beacon A with direction angle a and the line through B with angle b.
P intersect(P A, double a, P B, double b)
{
    // A + s (cos a, sin a) = B + t (cos b, sin b): a 2 x 2 linear system for s and t (Cramer's
    // rule).
    const double det = -std::cos(a) * std::sin(b) + std::sin(a) * std::cos(b);
    const double rx = B.x - A.x, ry = B.y - A.y;
    const double s = (-rx * std::sin(b) + ry * std::cos(b)) / det;
    return {A.x + s * std::cos(a), A.y + s * std::sin(a)};
}

int main()
{
    const double deg = std::numbers::pi / 180.0;
    const P A{0.0, 0.0}, B{10.0, 0.0}; // beacons, metres
    const P spots[] = {{5.0, 5.0}, {5.0, 0.3}};
    const double noise[5][2] = {{0.2, -0.2}, {-0.2, 0.2}, {0.2, 0.2}, {-0.2, -0.2}, {0.0, 0.0}};
    for (const P& r : spots) {
        const double a = std::atan2(r.y - A.y, r.x - A.x), b = std::atan2(r.y - B.y, r.x - B.x);
        std::printf("robot standing at (%.1f, %.1f); angle between the two sight lines %.2f deg\n",
                    r.x, r.y, std::fabs(b - a) / deg);
        for (const auto& n : noise) {
            const P p = intersect(A, a + n[0] * deg, B, b + n[1] * deg);
            std::printf("  fix: x = %8.3f  y = %8.3f   (bearing errors %+.1f, %+.1f deg)\n", p.x,
                        p.y, n[0], n[1]);
        }
    }
    return 0;
}
