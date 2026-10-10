// cspace.cpp - F9-55: the configuration space of a two-link planar arm.
// Links of 1.0 m and 0.8 m from a base at (0, 0); one box obstacle in the workspace.
// Each character is one configuration (theta1, theta2): '#' if any point of either link
// is inside the box, '.' if not. Then a straight joint-space move is checked.
#include <cmath>
#include <cstdio>
#include <string>

constexpr double kPi = 3.14159265358979323846;
constexpr double kL1 = 1.0;
constexpr double kL2 = 0.8;

bool inBox(double x, double y)
{
    return x >= 0.9 && x <= 1.3 && y >= 0.5 && y <= 1.1;
}

bool collides(double t1Deg, double t2Deg)
{
    const double t1 = t1Deg * kPi / 180.0;
    const double t2 = t2Deg * kPi / 180.0;
    const double ex = kL1 * std::cos(t1);                    // elbow
    const double ey = kL1 * std::sin(t1);
    for (int k = 0; k <= 40; ++k) {
        const double f = k / 40.0;
        if (inBox(f * ex, f * ey)) { return true; }          // link 1
        if (inBox(ex + f * kL2 * std::cos(t1 + t2), ey + f * kL2 * std::sin(t1 + t2))) { return true; }   // link 2
    }
    return false;
}

int main()
{
    int blocked = 0;
    int total = 0;
    std::printf("theta2 (deg) rows, theta1 (deg) columns from -180 to 175 in 5-degree steps\n");
    for (int t2 = 170; t2 >= -180; t2 -= 10) {
        std::string row;
        for (int t1 = -180; t1 < 180; t1 += 5) {
            const bool c = collides(t1, t2);
            row += c ? '#' : '.';
            blocked += c ? 1 : 0;
            ++total;
        }
        std::printf("%5d %s\n", t2, row.c_str());
    }
    std::printf("blocked %d of %d configurations (%.1f %%)\n", blocked, total, 100.0 * blocked / total);
    const double a[2] = {-30.0, 90.0};                        // A: upper arm down-right, elbow bent up
    const double b[2] = {90.0, -90.0};                        // B: arm pointing up, elbow bent right
    for (int k = 0; k <= 100; ++k) {
        const double f = k / 100.0;
        const double t1 = a[0] + f * (b[0] - a[0]);
        const double t2 = a[1] + f * (b[1] - a[1]);
        if (collides(t1, t2)) {
            std::printf("straight joint-space move A(-30,90) -> B(90,-90): first collision at %d %% "
                        "(theta1 %.1f, theta2 %.1f)\n", k, t1, t2);
            return 0;
        }
    }
    std::printf("straight joint-space move A(-30,90) -> B(90,-90): no collision\n");
    return 0;
}
