// F0-71 number check: recomputes the numbers used in the chapter text.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <vector>

int main()
{
    const double tm = 0.19608;
    const double ta = 0.01;
    const double tf = 0.02;
    const double kdc = 98.039;
    // Routh array for a3 s^3 + a2 s^2 + a1 s + a0.
    const double a3 = tm * ta * tf;
    const double a2 = tm * ta + tm * tf + ta * tf;
    const double a1 = tm + ta + tf;
    for (double kp : {0.05, 0.3475, 0.5}) {
        const double a0 = 1.0 + kp * kdc;
        const double b1 = (a2 * a1 - a3 * a0) / a2;
        std::cout << std::format("Kp = {}: Routh first column {:.4e}, {:.4e}, {:.5f}, {:.4f}\n", kp, a3, a2, b1, a0);
    }
    // Two-pole loop (motor + driver only): always stable for any Kp > 0.
    std::cout << std::format("motor + driver only: a2 = {:.6f}, a1 = {:.5f}: all coefficients positive for Kp > 0\n",
                             tm * ta, tm + ta);
    // Kp for 2 % error (F0-70) compared with the critical gain.
    std::cout << std::format("Kp for 2 % error = {:.4f}, critical gain = {:.4f}\n", 49.0 / kdc,
                             (a2 * a1 / a3 - 1.0) / kdc);
    // Pole of the first-order motor and its time constant.
    std::cout << std::format("motor pole -1/tm = {:.3f} 1/s\n", -1.0 / tm);
    // Gain margin at Kp = 0.05.
    std::cout << std::format("gain margin at Kp = 0.05: x{:.2f}\n", 0.3475 / 0.05);
    // Critical gain and crossing frequency for other filter lags (worked example, question 8).
    for (double tfx : {0.005, 0.04}) {
        const double c3 = tm * ta * tfx;
        const double c2 = tm * ta + tm * tfx + ta * tfx;
        const double c1 = tm + ta + tfx;
        std::cout << std::format("tf = {}: a3 = {:.4e}, a2 = {:.4e}, a1 = {:.5f}, limit {:.2f}, Kp crit = {:.4f}, "
                                 "crossing {:.1f} rad/s ({:.2f} Hz)\n",
                                 tfx, c3, c2, c1, c2 * c1 / c3, (c2 * c1 / c3 - 1.0) / kdc, std::sqrt(c1 / c3),
                                 std::sqrt(c1 / c3) / (2.0 * std::numbers::pi));
    }
    // Forensic: re-simulate the humming loop and measure the period and range after 2 s.
    const double kp = 0.5;
    const double dt = 1e-5;
    double u = 0.0;
    double w = 0.0;
    double y = 0.0;
    double wPrev = 0.0;
    double lo = 1e300;
    double hi = -1e300;
    std::vector<double> ups;
    const int n = static_cast<int>(std::lround(4.0 / dt));
    for (int k = 0; k <= n; ++k) {
        const double t = k * dt;
        const double command = std::clamp(kp * (100.0 - y), -12.0, 12.0);
        u += (command - u) / ta * dt;
        w += (kdc * u - w) / tm * dt;
        y += (w - y) / tf * dt;
        if (t >= 2.0) {
            lo = std::min(lo, w);
            hi = std::max(hi, w);
            if (wPrev < 100.0 && w >= 100.0) {
                ups.push_back(t);
            }
        }
        wPrev = w;
    }
    const double period = (ups.back() - ups.front()) / static_cast<double>(ups.size() - 1);
    std::cout << std::format("hum: speed {:.1f} to {:.1f} rad/s, period {:.4f} s ({:.2f} Hz) over {} cycles\n", lo, hi,
                             period, 1.0 / period, ups.size() - 1);
    std::cout << std::format("linear crossing frequency 75.93 rad/s = {:.2f} Hz\n", 75.93 / (2.0 * std::numbers::pi));
    return 0;
}
