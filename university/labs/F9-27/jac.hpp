// F9-27 Listing 1: the Jacobian of the planar 2R arm (L1, L2), and its uses.
#pragma once
#include <array>
#include <cmath>
#include <numbers>

using V2 = std::array<double, 2>;
using M2 = std::array<V2, 2>; // row-major 2 x 2

const double L1 = 0.30, L2 = 0.25; // link lengths of the simulated arm (m)

inline double deg(double r) // radians to degrees; + 0.0 prints -0 as 0
{
    return r * 180.0 / std::numbers::pi + 0.0;
}

inline V2 fk(const V2& q) // tool position for joint angles q = (t1, t2)
{
    return {L1 * std::cos(q[0]) + L2 * std::cos(q[0] + q[1]),
            L1 * std::sin(q[0]) + L2 * std::sin(q[0] + q[1])};
}

inline M2 jacobian(const V2& q) // J[i][j] = d fk_i / d q_j
{
    const double s1 = std::sin(q[0]), c1 = std::cos(q[0]);
    const double s12 = std::sin(q[0] + q[1]), c12 = std::cos(q[0] + q[1]);
    return {{{-L1 * s1 - L2 * s12, -L2 * s12}, {L1 * c1 + L2 * c12, L2 * c12}}};
}

inline double det(const M2& J)
{
    return J[0][0] * J[1][1] - J[0][1] * J[1][0];
}

inline V2 mul(const M2& A, const V2& v)
{
    return {A[0][0] * v[0] + A[0][1] * v[1], A[1][0] * v[0] + A[1][1] * v[1]};
}

inline M2 transpose(const M2& A)
{
    return {{{A[0][0], A[1][0]}, {A[0][1], A[1][1]}}};
}

// joint velocities for a wanted tool velocity: solve J qdot = v (Cramer's rule)
inline V2 solve(const M2& J, const V2& v)
{
    const double d = det(J);
    return {(J[1][1] * v[0] - J[0][1] * v[1]) / d, (-J[1][0] * v[0] + J[0][0] * v[1]) / d};
}

// damped least squares: qdot = J^T (J J^T + lambda^2 I)^-1 v, bounded near singularities
inline V2 solveDamped(const M2& J, const V2& v, double lambda)
{
    const M2 Jt = transpose(J);
    M2 A{};
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            A[i][j] = J[i][0] * Jt[0][j] + J[i][1] * Jt[1][j] + (i == j ? lambda * lambda : 0);
    const double d = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    const V2 y{(A[1][1] * v[0] - A[0][1] * v[1]) / d, (-A[1][0] * v[0] + A[0][0] * v[1]) / d};
    return mul(Jt, y);
}
