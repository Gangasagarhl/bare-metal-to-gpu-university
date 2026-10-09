// F0-78 worked example check: a nearly singular 2 x 2 system and its condition number.
#include <cmath>
#include <cstdio>

int main()
{
    // A = [[1, 1], [1, 1.0001]]; solve by Cramer's rule (fine for 2 x 2 teaching examples).
    const double a = 1, b = 1, c = 1, d = 1.0001;
    const double det = a * d - b * c;
    auto solve = [&](double r1, double r2, double* x1, double* x2) {
        *x1 = (r1 * d - b * r2) / det;
        *x2 = (a * r2 - c * r1) / det;
    };
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    solve(2.0, 2.0001, &x1, &x2);
    std::printf("b = (2, 2.0001)  -> x = (%.6f, %.6f)\n", x1, x2);
    solve(2.0, 2.0002, &y1, &y2);
    std::printf("b = (2, 2.0002)  -> x = (%.6f, %.6f)\n", y1, y2);
    const double normA = std::fmax(std::fabs(a) + std::fabs(b), std::fabs(c) + std::fabs(d));
    const double normInv =
        std::fmax(std::fabs(d) + std::fabs(b), std::fabs(c) + std::fabs(a)) / std::fabs(det);
    std::printf("det = %.6g, ||A||_inf = %.6g, ||A^-1||_inf = %.6g, kappa_inf = %.6g\n", det, normA,
                normInv, normA * normInv);
    const double dx =
        std::fmax(std::fabs(y1 - x1), std::fabs(y2 - x2)) / std::fmax(std::fabs(x1), std::fabs(x2));
    std::printf("relative change of b (inf-norm): %.3g; relative change of x: %.3g\n",
                0.0001 / 2.0001, dx);
    return 0;
}
