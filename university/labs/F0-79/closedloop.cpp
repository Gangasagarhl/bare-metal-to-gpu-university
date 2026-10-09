// F0-79 Layer 3 check: closed-loop poles of the sampled PI loop of Listing 1.
// Plant with zero-order hold: w[k+1] = a w[k] + b u[k], a = e^(-T/tau), b = (1 - a) K.
// Controller, incremental form: u[k] = u[k-1] + q0 e[k] + q1 e[k-1].
// Characteristic polynomial: z^2 + (b q0 - 1 - a) z + (a + b q1) = 0. Stable when both |z| < 1.
#include <cmath>
#include <complex>
#include <cstdio>

int main()
{
    const double tau = 0.5, K = 2.0, kp = 2.0, ki = 8.0;
    // Continuous loop: tau s^2 + (1 + K kp) s + K ki = 0; mapped poles z = e^(sT) for comparison.
    const std::complex<double> disc =
        std::sqrt(std::complex<double>((1 + K * kp) * (1 + K * kp) - 4 * tau * K * ki, 0.0));
    const std::complex<double> s1 = (-(1 + K * kp) + disc) / (2 * tau);
    std::printf("continuous closed-loop poles s = %.4f %+.4fi (and conjugate)\n", s1.real(),
                std::fabs(s1.imag()));
    std::printf("%-6s %-9s %-7s %-7s %-22s %-9s %s\n", "T s", "rule", "q0", "q1", "poles z",
                "max|z|", "|e^(sT)|");
    const double periods[] = {0.01, 0.05, 0.1, 0.2};
    const char* names[] = {"forward", "backward", "Tustin"};
    for (double T : periods) {
        const double a = std::exp(-T / tau), b = (1 - a) * K;
        const double q0s[] = {kp, kp + ki * T, kp + ki * T / 2};
        const double q1s[] = {-kp + ki * T, -kp, -kp + ki * T / 2};
        for (int r = 0; r < 3; ++r) {
            const double B = b * q0s[r] - 1 - a, C = a + b * q1s[r];
            const std::complex<double> d = std::sqrt(std::complex<double>(B * B - 4 * C, 0.0));
            const std::complex<double> z1 = (-B + d) / 2.0, z2 = (-B - d) / 2.0;
            char poles[64];
            if (std::fabs(z1.imag()) > 1e-12) {
                std::snprintf(poles, sizeof poles, "%.3f +/- %.3fi", z1.real(),
                              std::fabs(z1.imag()));
            } else {
                std::snprintf(poles, sizeof poles, "%.3f, %.3f", z1.real(), z2.real());
            }
            std::printf("%-6.3f %-9s %-7.3f %-7.3f %-22s %-9.4f %.4f\n", T, names[r], q0s[r],
                        q1s[r], poles, std::fmax(std::abs(z1), std::abs(z2)),
                        std::exp(s1.real() * T));
        }
    }
    return 0;
}
