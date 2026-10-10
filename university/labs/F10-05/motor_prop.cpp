// motor_prop.cpp - a motor and propeller model of the course quad (F10-05).
// Motor: V = R i + Ke w (back-EMF), torque Kt i. Propeller: thrust kT w^2, drag torque kQ w^2.
// Rotor: Jr dw/dt = Kt i - kQ w^2. All values are exercise values, not a real motor's.
#include <cmath>
#include <cstdio>
#include <initializer_list>

namespace {

constexpr double R = 0.1;      // ohm: winding resistance
constexpr double Ke = 0.01;    // V per rad/s (back-EMF constant)
constexpr double Kt = 0.01;    // N m per A (torque constant, equal to Ke in SI units)
constexpr double Jr = 2.0e-5;  // kg m^2: rotor plus propeller inertia
constexpr double kT = 1.0e-5;  // N per (rad/s)^2
constexpr double kQ = 1.5e-7;  // N m per (rad/s)^2

double steadySpeed(double volts)  // solves Kt (V - Ke w) / R = kQ w^2 for w >= 0
{
    const double a = kQ * R, b = Kt * Ke, c = -Kt * volts;
    return (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
}

double accel(double volts, double w)
{
    const double current = (volts - Ke * w) / R;
    return (Kt * current - kQ * w * w) / Jr;
}

} // namespace

int main()
{
    std::printf("volts  speed(rad/s)  thrust(N)  current(A)  power in(W)  thrust per watt(N/W)\n");
    for (double v : {2.0, 4.0, 6.0, 8.0, 10.0, 12.0}) {
        const double w = steadySpeed(v);
        const double i = (v - Ke * w) / R;
        std::printf("%5.1f %13.1f %10.3f %11.2f %12.2f %20.4f\n", v, w, kT * w * w, i, v * i,
                    kT * w * w / (v * i));
    }
    const double hover = std::sqrt(9.81 / 4.0 / kT);
    const double vHover = (kQ * R * hover * hover + Kt * Ke * hover) / Kt;
    std::printf("hover: %.1f rad/s per rotor needs %.3f V\n", hover, vHover);

    // Step response: from hover voltage, add 1 V and watch the speed (Euler steps of 10 us).
    const double target = steadySpeed(vHover + 1.0);
    const double tauLinear = Jr / (Kt * Ke / R + 2.0 * kQ * hover);
    double w = hover, t = 0.0, t63 = -1.0;
    const double dt = 1e-5;
    std::printf("step +1 V: speed goes from %.1f to %.1f rad/s\n", hover, target);
    std::printf("  t(ms)  speed(rad/s)\n");
    for (int k = 0; k <= 10000; ++k) {
        if (k % 1000 == 0) {
            std::printf("%7.1f %13.1f\n", t * 1000.0, w);
        }
        if (t63 < 0.0 && w >= hover + 0.632 * (target - hover)) {
            t63 = t;
        }
        w += dt * accel(vHover + 1.0, w);
        t += dt;
    }
    std::printf("time to 63.2 %% of the change: %.2f ms (linearised model: %.2f ms)\n",
                t63 * 1000.0, tauLinear * 1000.0);
    return 0;
}
