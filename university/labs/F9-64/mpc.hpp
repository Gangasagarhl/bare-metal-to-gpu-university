// mpc.hpp - a teaching model predictive controller (F9-64): linear model, quadratic cost,
// box limits on the input and a soft upper limit on the first state. The QP is solved by an
// accelerated projected-gradient method (FISTA). Not tuned for speed or for real-time use.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "lin.hpp"

struct Mpc
{
    Mat H;            // Hessian of the cost in U (N x N), without the soft limit
    Mat F;            // linear term: gradient = 2 (H U + F x0) + soft-limit term
    Mat Sx, Su;       // predicted states X = Sx x0 + Su U, stacked (x1 ... xN)
    std::size_t n = 0;
    std::size_t N = 0;
    double umax = 0.0;
    double x1max = 0.0;    // soft limit on state 0 (a wall), with weight rho
    double rho = 0.0;
    double step = 0.0;     // 1 / Lipschitz constant of the gradient
    std::vector<double> warm;  // last solution, shifted, as the next starting point
};

// Build the condensed QP for horizon N with stage weights Q, R and terminal weight P.
inline Mpc makeMpc(const Mat& A, const Mat& B, const Mat& Q, double R, const Mat& P,
                   std::size_t N, double umax, double x1max, double rho)
{
    Mpc c;
    c.n = A.r;
    c.N = N;
    c.umax = umax;
    c.x1max = x1max;
    c.rho = rho;
    const std::size_t n = A.r;
    c.Sx = Mat(n * N, n);
    c.Su = Mat(n * N, N);
    Mat Ak = eye(n);
    std::vector<Mat> AkB;  // A^j B for j = 0 .. N-1
    Mat blk = B;
    for (std::size_t j = 0; j < N; ++j) {
        AkB.push_back(blk);
        blk = A * blk;
    }
    for (std::size_t k = 0; k < N; ++k) {  // row block k holds x_{k+1}
        Ak = A * Ak;
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) c.Sx(k * n + i, j) = Ak(i, j);
            for (std::size_t j = 0; j <= k; ++j) c.Su(k * n + i, j) = AkB[k - j](i, 0);
        }
    }
    Mat Qbar(n * N, n * N);
    for (std::size_t k = 0; k < N; ++k)
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < n; ++j)
                Qbar(k * n + i, k * n + j) = (k + 1 == N ? P : Q)(i, j);
    c.H = tr(c.Su) * Qbar * c.Su + R * eye(N);
    c.F = tr(c.Su) * Qbar * c.Sx;
    // Lipschitz constant: 2 * (largest eigenvalue of H + rho * Su0^T Su0), by power iteration
    Mat G = c.H;
    for (std::size_t a = 0; a < N; ++a)
        for (std::size_t b = 0; b < N; ++b)
            for (std::size_t k = 0; k < N; ++k) G(a, b) += rho * c.Su(k * n, a) * c.Su(k * n, b);
    Mat v(N, 1, 1.0);
    double lam = 0.0;
    for (int it = 0; it < 200; ++it) {
        const Mat w = G * v;
        lam = std::sqrt((tr(w) * w)(0, 0));
        v = (1.0 / lam) * w;
    }
    c.step = 1.0 / (2.0 * lam);
    c.warm.assign(N, 0.0);
    return c;
}

// Solve for the input sequence from state x0; returns U (first entry is applied).
inline std::vector<double> solve(Mpc& c, const Mat& x0, int iters)
{
    const std::size_t N = c.N;
    const Mat Fx = c.F * x0;
    const Mat X0 = c.Sx * x0;
    std::vector<double> u = c.warm, y = u, uPrev = u;
    double t = 1.0;
    for (int it = 0; it < iters; ++it) {
        const Mat Y(N, 1, y);
        const Mat g = 2.0 * (c.H * Y + Fx);
        std::vector<double> grad = g.a;
        for (std::size_t k = 0; k < N; ++k) {  // soft wall on state 0 of x_{k+1}
            double xk = X0(k * c.n, 0);
            for (std::size_t j = 0; j <= k; ++j) xk += c.Su(k * c.n, j) * y[j];
            const double over = xk - c.x1max;
            if (over > 0.0)
                for (std::size_t j = 0; j <= k; ++j)
                    grad[j] += 2.0 * c.rho * over * c.Su(k * c.n, j);
        }
        uPrev = u;
        for (std::size_t j = 0; j < N; ++j)
            u[j] = std::clamp(y[j] - c.step * grad[j], -c.umax, c.umax);  // project onto the box
        const double tn = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * t * t));
        for (std::size_t j = 0; j < N; ++j) y[j] = u[j] + (t - 1.0) / tn * (u[j] - uPrev[j]);
        t = tn;
    }
    for (std::size_t j = 0; j + 1 < N; ++j) c.warm[j] = u[j + 1];  // shift for next time
    c.warm[N - 1] = u[N - 1];
    return u;
}
