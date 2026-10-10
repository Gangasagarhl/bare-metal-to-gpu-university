// F0-79 forensic model: a joint position loop (shared by buzz.cpp and buzz_fix.cpp).
// Plant model (lab numbers, not a real motor): theta'' = (-theta' + K u) / tau.
// Controller: u = Kp e + Kd (e - e_prev) / dt, with dt written into the code as a constant
// (the deliberate mistake; see the answer key). The motor is simulated with fine RK4 steps between
// samples.
#pragma once
#include <cmath>
#include <cstdio>

inline constexpr double kTau = 0.05;       // s
inline constexpr double kK = 20.0;         // (rad/s) per volt
inline constexpr double kKp = 8.0;         // V/rad
inline constexpr double kKd = 0.12;        // V s/rad
inline constexpr double kDtInCode = 0.001; // s: the period the gains were tuned for
inline constexpr double kUmax = 12.0;      // V: the driver's output limit in this model

struct S
{
    double th, w;
};

inline S f(const S& s, double u)
{
    return {s.w, (-s.w + kK * u) / kTau};
}

inline S rk4(const S& s, double u, double h)
{
    const S a = f(s, u);
    const S b = f({s.th + h / 2 * a.th, s.w + h / 2 * a.w}, u);
    const S c = f({s.th + h / 2 * b.th, s.w + h / 2 * b.w}, u);
    const S d = f({s.th + h * c.th, s.w + h * c.w}, u);
    return {s.th + h / 6 * (a.th + 2 * b.th + 2 * c.th + d.th),
            s.w + h / 6 * (a.w + 2 * b.w + 2 * c.w + d.w)};
}

inline void run(const char* label, double period, double dtInCode)
{
    S s{0.0, 0.0};
    const double target = 1.0; // rad, step at t = 0
    double ePrev = target;
    std::printf("%s: loop period %.0f ms, dt in code %.0f ms\n", label, period * 1000,
                dtInCode * 1000);
    std::printf("  %-7s %-9s %-8s\n", "t ms", "theta", "u V");
    const int samples = static_cast<int>(0.4 / period + 0.5);
    const int sub = static_cast<int>(period / 1.0e-5 + 0.5);
    for (int k = 0; k <= samples; ++k) {
        const double e = target - s.th;
        double u = kKp * e + kKd * (e - ePrev) / dtInCode;
        u = std::fmax(-kUmax, std::fmin(kUmax, u));
        ePrev = e;
        const int t_ms = static_cast<int>(k * period * 1000 + 0.5);
        if (t_ms % 10 == 0 && t_ms <= 150) {
            std::printf("  %-7d %-9.4f %-8.2f\n", t_ms, s.th, u);
        }
        for (int i = 0; i < sub; ++i) {
            s = rk4(s, u, period / sub);
        }
    }
}
