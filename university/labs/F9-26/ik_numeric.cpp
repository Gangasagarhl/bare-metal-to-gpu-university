// F9-26 Listing 3: numerical IK for the 3D course arm of F9-25 (damped least squares),
// compared with the analytic answer (base yaw, then the 2R solution in the arm's plane).
#include "ik.hpp"
#include <array>
#include <cstdio>

using V3 = std::array<double, 3>;
const double H0 = 0.10, A1 = 0.30, A2 = 0.30; // shoulder height, upper arm, forearm + tool (m)

V3 fk(const V3& q) // course arm tool position (closed form from F9-25)
{
    const double reach = A1 * std::cos(q[1]) + A2 * std::cos(q[1] + q[2]);
    return {reach * std::cos(q[0]), reach * std::sin(q[0]),
            H0 + A1 * std::sin(q[1]) + A2 * std::sin(q[1] + q[2])};
}

V3 solve3(std::array<V3, 3> A, V3 b) // Gaussian elimination with partial pivoting
{
    for (int c = 0; c < 3; ++c) {
        int p = c;
        for (int r = c + 1; r < 3; ++r)
            if (std::abs(A[r][c]) > std::abs(A[p][c])) p = r;
        std::swap(A[c], A[p]);
        std::swap(b[c], b[p]);
        for (int r = c + 1; r < 3; ++r) {
            const double f = A[r][c] / A[c][c];
            for (int k = c; k < 3; ++k) A[r][k] -= f * A[c][k];
            b[r] -= f * b[c];
        }
    }
    V3 x{};
    for (int r = 2; r >= 0; --r) {
        double s = b[r];
        for (int k = r + 1; k < 3; ++k) s -= A[r][k] * x[k];
        x[r] = s / A[r][r];
    }
    return x;
}

int main()
{
    const V3 goal{0.25, 0.30, 0.35};
    V3 q{0.0, 0.3, 0.3};        // starting guess (rad)
    const double lambda = 0.01; // damping
    std::printf("iter   q1 (deg)  q2 (deg)  q3 (deg)   position error (m)\n");
    for (int it = 0; it <= 30; ++it) {
        const V3 p = fk(q);
        const V3 e{goal[0] - p[0], goal[1] - p[1], goal[2] - p[2]};
        const double err = std::hypot(e[0], e[1], e[2]);
        std::printf("%3d   %8.3f  %8.3f  %8.3f   %.3e\n", it, deg(q[0]), deg(q[1]), deg(q[2]), err);
        if (err < 1e-12) break;
        std::array<V3, 3> J{}; // Jacobian by finite differences (F9-27 derives it exactly)
        const double h = 1e-7;
        for (int j = 0; j < 3; ++j) {
            V3 qh = q;
            qh[j] += h;
            const V3 ph = fk(qh);
            for (int i = 0; i < 3; ++i) J[i][j] = (ph[i] - p[i]) / h;
        }
        std::array<V3, 3> A{}; // (J^T J + lambda^2 I) dq = J^T e
        V3 b{};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) A[i][j] += J[k][i] * J[k][j];
            }
            A[i][i] += lambda * lambda;
            for (int k = 0; k < 3; ++k) b[i] += J[k][i] * e[k];
        }
        V3 dq = solve3(A, b);
        const double big = std::max({std::abs(dq[0]), std::abs(dq[1]), std::abs(dq[2])});
        const double maxStep = 0.2; // rad: never trust the linear model for a big jump
        for (int j = 0; j < 3; ++j) q[j] += big > maxStep ? dq[j] * maxStep / big : dq[j];
    }
    // analytic answer for comparison: yaw towards the goal, then 2R in the vertical plane
    const double q1 = std::atan2(goal[1], goal[0]);
    for (bool up : {true, false}) {
        const auto a = ik2r(A1, A2, std::hypot(goal[0], goal[1]), goal[2] - H0, up);
        if (a)
            std::printf("analytic (%s): q1 %8.3f  q2 %8.3f  q3 %8.3f\n",
                        up ? "elbow-up  " : "elbow-down", deg(q1), deg(a->t1), deg(a->t2));
    }
    return 0;
}
