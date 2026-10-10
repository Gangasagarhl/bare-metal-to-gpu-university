// checks.cpp - recomputes the numbers used in the text of F10-04.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double g = 9.81, m = 1.0, arm = 0.15, Jx = 0.010, kick = 0.2;
    const double deg = std::numbers::pi / 180.0;
    // sim2d: roll pulse 0.2 s at torque arm*(2 kick), then the opposite 0.2 s
    const double alpha = arm * 2 * kick / Jx;
    const double phi = alpha * 0.2 * 0.2;  // two phases of 0.2 s: 1/2 a t^2 twice = a t^2
    std::printf("roll acceleration %.1f rad/s^2, peak rate %.2f rad/s, final roll %.4f rad = %.2f deg\n",
                alpha, alpha * 0.2, phi, phi / deg);
    std::printf("sideways acceleration after the pulse: %.3f m/s^2\n", -g * std::tan(phi));
    // T3 of the tests
    const double r = 20 * deg, p = -10 * deg;
    const double T = m * g / (std::cos(r) * std::cos(p));
    std::printf("T3: thrust %.4f N, accel (%.4f, %.4f) m/s^2\n", T,
                T * std::cos(r) * std::sin(p) / m, -T * std::sin(r) / m);
    // torque-free spin of the forensic program: w = (2, 10, 1), J = (0.010, 0.014, 0.018)
    const double wx = 2, wy = 10, wz = 1, J1 = 0.010, J2 = 0.014, J3 = 0.018;
    std::printf("forensic: |L| = %.5f, energy = %.6f J\n",
                std::sqrt(std::pow(J1 * wx, 2) + std::pow(J2 * wy, 2) + std::pow(J3 * wz, 2)),
                0.5 * (J1 * wx * wx + J2 * wy * wy + J3 * wz * wz));
    std::printf("forensic: w x Jw = (%.4f, %.4f, %.4f) N m equivalent\n",
                wy * J3 * wz - wz * J2 * wy, wz * J1 * wx - wx * J3 * wz, wx * J2 * wy - wy * J1 * wx);
    std::printf("hover rotor speed %.2f rad/s; roll angular accel from +-20 rad/s on the left/right "
                "pairs: about %.1f rad/s^2\n", std::sqrt(m * g / 4 / 1e-5),
                arm * 4 * 1e-5 * 2 * 495.23 * 20 / Jx);
    return 0;
}
