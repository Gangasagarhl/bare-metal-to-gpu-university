// F0-60 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>

int main()
{
    // Worked example (covariance.in): Sxy = 26, Sxx = 42, Syy = 17.3333, n - 1 = 5.
    const double sxy = 9.33333333 + 2.66666667 + 0.33333333 - 0.33333333 + 3.33333333 + 10.6666667;
    const double sxx = 16 + 4 + 1 + 1 + 4 + 16;
    const double syy = (49.0 + 16.0 + 1.0 + 1.0 + 25.0 + 64.0) / 9.0;
    std::cout << "Sxy = " << sxy << ", Sxx = " << sxx << ", Syy = " << syy << "\n";
    std::cout << "cov = " << sxy / 5 << ", r = " << sxy / std::sqrt(sxx * syy) << "\n";
    std::cout << "best-fit slope = cov / var x = " << sxy / sxx << " C per hour, intercept = "
              << 21.0 + 1.0 / 3.0 - (sxy / sxx) * 6.0 << " C\n";
    // Variance of a sum and of a difference with covariance.
    const double vx = 4.0;
    const double vy = 9.0;
    const double c = 3.0;
    std::cout << "Var(X+Y) = 4 + 9 + 2*3 = " << vx + vy + 2 * c << ", Var(X-Y) = "
              << vx + vy - 2 * c << ", r = " << c / std::sqrt(vx * vy) << "\n";
    // Average of two readings with sd s and correlation rho: sd = s * sqrt((1 + rho) / 2).
    for (double rho : {0.0, 0.5, 0.88, 1.0}) {
        std::cout << "rho = " << rho << ": sd of average / sd of one = " << std::sqrt((1 + rho) / 2)
                  << "\n";
    }
    // Forensic: cov from the measured sds.
    const double sa = 0.284509;
    const double sb = 0.285901;
    const double sm = 0.275964;
    const double cov = (4 * sm * sm - sa * sa - sb * sb) / 2;
    std::cout << "forensic: cov = " << cov << ", r = " << cov / (sa * sb) << "\n";
    std::cout << "model: shared sd 0.27, own sd 0.10 -> r = " << 0.27 * 0.27 / (0.27 * 0.27 + 0.01)
              << ", sd one = " << std::sqrt(0.0829) << ", sd avg = "
              << std::sqrt(0.0729 + 0.01 / 2) << ", sd(A-B) = " << std::sqrt(0.02) << "\n";
    // Check yourself: unit change does not change r; cov scales.
    std::cout << "cov in Fahrenheit = 1.8 * 5.2 = " << 1.8 * 5.2 << "\n";
    return 0;
}
