// F9-17 forensic evidence generator: "The motor that buzzes". One row per control
// period. The change that was made is described only in the answer key.
#include <algorithm>
#include <format>
#include <iostream>

struct Motor
{
    double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    double w = 0.0;

    void step(double volts, double dt) { w += dt * (kdc * volts - w) / tau; }
};

int main()
{
    Motor m;
    std::cout << " sample  t (s)    Kp   speed (rad/s)  error  command (V)\n";
    for (int k = 0; k <= 130; ++k) {
        const double t = k * 0.01;
        const double kp = (t < 1.0 - 1e-9) ? 0.2 : 0.45;
        const double error = 300.0 - m.w;
        const double volts = std::clamp(kp * error, -12.0, 12.0);
        if (k >= 97) {
            std::cout << std::format("{:>7} {:>6.2f} {:>5.2f} {:>15.1f} {:>6.1f} {:>12.2f}\n", k, t,
                                     kp, m.w, error, volts);
        }
        for (int s = 0; s < 100; ++s) {
            m.step(volts, 0.0001);
        }
    }
    return 0;
}
