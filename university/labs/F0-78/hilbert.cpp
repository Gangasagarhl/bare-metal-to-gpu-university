// F0-78 Listing 1: a stable solver on ill-conditioned matrices. The residual stays tiny; the error
// does not.
#include <cmath>
#include <cstdio>

#include "solve.hpp"

// Infinity norm of a matrix: the largest row sum of absolute values.
double normInf(const Mat& A)
{
    double worst = 0.0;
    for (const Vec& row : A) {
        double s = 0.0;
        for (double v : row) {
            s += std::fabs(v);
        }
        worst = std::fmax(worst, s);
    }
    return worst;
}

int main()
{
    std::printf("%-3s %-12s %-14s %-14s %-12s\n", "n", "kappa_inf", "rel. error", "rel. residual",
                "kappa * u");
    const double u = std::ldexp(1.0, -53);
    for (std::size_t n = 2; n <= 13; ++n) {
        Mat H(n, Vec(n));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                H[i][j] = 1.0 / static_cast<double>(i + j + 1); // Hilbert matrix
            }
        }
        const Vec xTrue(n, 1.0);
        Vec b(n, 0.0);
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                b[i] += H[i][j] * xTrue[j];
            }
        }
        const Vec x = solve(H, b, true, false);
        // Inverse, column by column (solve H y = e_j), for kappa = ||H|| ||H^-1||.
        Mat inv(n, Vec(n));
        for (std::size_t j = 0; j < n; ++j) {
            Vec e(n, 0.0);
            e[j] = 1.0;
            const Vec col = solve(H, e, true, false);
            for (std::size_t i = 0; i < n; ++i) {
                inv[i][j] = col[i];
            }
        }
        const double kappa = normInf(H) * normInf(inv);
        double err = 0.0, bmax = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            err = std::fmax(err, std::fabs(x[i] - xTrue[i]));
            bmax = std::fmax(bmax, std::fabs(b[i]));
        }
        std::printf("%-3zu %-12.3e %-14.3e %-14.3e %-12.3e\n", n, kappa, err,
                    residualMax(H, x, b) / bmax, kappa * u);
    }
    return 0;
}
