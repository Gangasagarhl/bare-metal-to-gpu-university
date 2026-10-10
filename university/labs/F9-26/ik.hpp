// F9-26 Listing 1: analytic inverse kinematics of a planar two-link (2R) arm.
#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

struct Angles2 {
    double t1, t2; // shoulder and elbow angles (rad)
};

inline double deg(double r) // radians to degrees; + 0.0 prints -0 as 0
{
    return r * 180.0 / std::numbers::pi + 0.0;
}

inline void fk2r(double L1, double L2, const Angles2& a, double& x, double& y)
{
    x = L1 * std::cos(a.t1) + L2 * std::cos(a.t1 + a.t2);
    y = L1 * std::sin(a.t1) + L2 * std::sin(a.t1 + a.t2);
}

// elbowUp selects the branch with t2 <= 0 (elbow above the shoulder-target line
// for targets in front of the arm); the other branch has t2 >= 0.
inline std::optional<Angles2> ik2r(double L1, double L2, double x, double y, bool elbowUp)
{
    const double c2 = (x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2); // law of cosines
    const double tol = 1e-9;                                // accept rounding at the edge of reach
    if (c2 > 1 + tol || c2 < -1 - tol) return std::nullopt; // target out of reach
    const double c = std::clamp(c2, -1.0, 1.0); // keep sqrt and acos inside their domain
    const double s2 = (elbowUp ? -1.0 : 1.0) * std::sqrt(1 - c * c);
    const double t2 = std::atan2(s2, c);
    const double t1 = std::atan2(y, x) - std::atan2(L2 * s2, L1 + L2 * c);
    return Angles2{t1, t2};
}
