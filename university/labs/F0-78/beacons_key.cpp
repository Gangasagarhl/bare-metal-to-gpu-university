// F0-78 forensic answer-key run: condition number of the 2 x 2 system behind each fix,
// and the position change per degree of bearing error.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double deg = std::numbers::pi / 180.0;
    const double spots[][2] = {{5.0, 5.0}, {5.0, 0.3}, {5.0, 1.0}, {5.0, 2.5}};
    std::printf("%-12s %-14s %-12s %-22s\n", "robot at", "angle (deg)", "kappa_2",
                "x shift per 0.2 deg (m)");
    for (const auto& r : spots) {
        const double a = std::atan2(r[1], r[0]), b = std::atan2(r[1], r[0] - 10.0);
        // Columns of the system matrix are the two unit direction vectors d_a and -d_b.
        const double m11 = std::cos(a), m12 = -std::cos(b), m21 = std::sin(a), m22 = -std::sin(b);
        // 2-norm condition number from the singular values of a 2 x 2 matrix.
        const double t = m11 * m11 + m12 * m12 + m21 * m21 + m22 * m22;
        const double det = m11 * m22 - m12 * m21;
        const double disc = std::sqrt(t * t - 4.0 * det * det);
        const double smax = std::sqrt((t + disc) / 2.0), smin = std::sqrt((t - disc) / 2.0);
        // Same-sign errors on both bearings: intersect the rotated lines.
        const double a2 = a + 0.2 * deg, b2 = b + 0.2 * deg;
        const double dd = -std::cos(a2) * std::sin(b2) + std::sin(a2) * std::cos(b2);
        const double s = (-10.0 * std::sin(b2)) / dd;
        std::printf("(%.1f, %.1f)   %-14.2f %-12.2f %-22.3f\n", r[0], r[1], std::fabs(b - a) / deg,
                    smax / smin, s * std::cos(a2) - r[0]);
    }
    return 0;
}
