// checks.cpp - recomputes the numbers used in the text of F10-05.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double R = 0.1, Ke = 0.01, Kt = 0.01, Jr = 2.0e-5, kT = 1.0e-5, kQ = 1.5e-7;
    const double w0 = std::sqrt(9.81 / 4.0 / kT);
    const double i0 = kQ * w0 * w0 / Kt;
    const double v0 = R * i0 + Ke * w0;
    std::printf("hover: w %.1f rad/s, current %.3f A, voltage %.3f V\n", w0, i0, v0);
    std::printf("hover: electrical power %.2f W per motor, %.1f W for four\n", v0 * i0, 4 * v0 * i0);
    std::printf("hover: mechanical power Q w = %.2f W, copper loss i^2 R = %.3f W, ratio %.3f\n",
                kQ * w0 * w0 * w0, i0 * i0 * R, kQ * w0 * w0 * w0 / (v0 * i0));
    std::printf("thrust per watt = kT Kt / (kQ V) = %.4f / V\n", kT * Kt / kQ);
    std::printf("linearised tau at hover: %.2f ms\n", 1000 * Jr / (Kt * Ke / R + 2 * kQ * w0));
    std::printf("tau without propeller drag (Jr R / (Kt Ke)): %.1f ms\n", 1000 * Jr * R / (Kt * Ke));
    std::printf("no-load speed at 12 V (V / Ke): %.0f rad/s = %.0f rpm\n", 12 / Ke,
                12 / Ke * 60 / (2 * std::numbers::pi));
    std::printf("Kv equivalent of Ke = 0.01 V s/rad: %.1f rpm per volt\n",
                60.0 / (2 * std::numbers::pi * Ke));
    std::printf("double thrust from hover: w x %.4f, mech power x %.4f\n", std::sqrt(2.0),
                std::pow(2.0, 1.5));
    // forensic: line T = a w + b with a, b printed by stand_linear
    const double a = 0.010017, b = -1.8282;
    std::printf("line at 0 rad/s: %.4f N; line hover speed %.1f; true thrust there %.3f N\n", b,
                (2.4525 - b) / a, kT * std::pow((2.4525 - b) / a, 2));
    std::printf("quadratic model at 427.4 rad/s: %.3f N (shortfall %.3f N per rotor, %.2f N total)\n",
                1.0035e-5 * 427.4 * 427.4, 2.4525 - 1.0035e-5 * 427.4 * 427.4,
                4 * (2.4525 - 1.0035e-5 * 427.4 * 427.4));
    return 0;
}
