// F0-70 number check: recomputes the numbers used in the chapter text.
#include <format>
#include <iostream>

int main()
{
    const double r = 2.0;
    const double l = 1e-3;
    const double k = 0.01;
    const double j = 1e-5;
    const double b = 1e-6;
    // Full motor transfer function K / (L J s^2 + (R J + L b) s + (R b + K^2)).
    std::cout << std::format("full motor G(s) = {} / ({:.4g} s^2 + {:.6g} s + {:.4g})\n", k, l * j, r * j + l * b,
                             r * b + k * k);
    const double kdc = k / (k * k + r * b);
    const double tau = j * r / (k * k + r * b);
    std::cout << std::format("Kdc = {:.3f} rad/s per V, tau_m = {:.5f} s\n", kdc, tau);
    for (double kp : {0.01, 0.05, 0.2}) {
        const double loop = kp * kdc;
        std::cout << std::format(
            "Kp = {}: loop gain {:.4f}, T(0) = {:.4f}, error fraction 1/(1+L) = {:.4f}, tau_cl = {:.5f} s, "
            "speed-up x{:.3f}, first volts at r = 300: {:.1f}\n",
            kp, loop, loop / (1.0 + loop), 1.0 / (1.0 + loop), tau / (1.0 + loop), 1.0 + loop, kp * 300.0);
    }
    // Kp needed for a 2 % steady-state error.
    std::cout << std::format("Kp for 2 % error: (1/0.02 - 1) / Kdc = {:.4f}\n", (1.0 / 0.02 - 1.0) / kdc);
    // Forensic: error = measured - setpoint gives tau w' = (Kp Kdc - 1) w - Kp Kdc r.
    const double kp = 0.05;
    const double loop = kp * kdc;
    std::cout << std::format("forensic: pole at +(Kp Kdc - 1)/tau = {:.3f} 1/s, doubling time {:.4f} s\n",
                             (loop - 1.0) / tau, 0.693147 * tau / (loop - 1.0));
    std::cout << std::format("forensic: unstable balance point w = r Kp Kdc / (Kp Kdc - 1) = {:.4f} r\n",
                             loop / (loop - 1.0));
    std::cout << std::format("forensic: steady speed at -12 V = {:.2f} rad/s\n", -12.0 * kdc);
    std::cout << std::format("forensic: command at t = 0, r = 100: {:.2f} V\n", kp * (0.0 - 100.0));
    return 0;
}
