// F9-17 Listing 1: proportional speed control of the pretend motor, many gains.
// Controller every T = 10 ms; the voltage is held between samples (as a real driver does).
// For each gain: the settled speed, the error left over, the error the formula predicts,
// the sample-to-sample pole of the loop, and the largest overshoot seen.
#include <algorithm>
#include <array>
#include <cmath>
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
    const double setpoint = 300.0;
    const double period = 0.01;
    const int subSteps = 100;
    const Motor model;
    const double a = std::exp(-period / model.tau); // fraction of the old speed kept per sample
    const double b = model.kdc * (1.0 - a);         // speed gained per volt per sample
    std::cout << std::format("per sample: a = {:.4f}, b = {:.4f} rad/s per V\n", a, b);
    std::cout << "   Kp    settled  error  predicted  pole    max speed  last 4 commands (V)\n";
    for (double kp : {0.01, 0.05, 0.1, 0.2, 0.3, 0.38, 0.45}) {
        Motor m;
        double maxSpeed = 0.0;
        std::array<double, 400> volts{};
        for (int k = 0; k < 400; ++k) {
            volts[k] = std::clamp(kp * (setpoint - m.w), -12.0, 12.0);
            for (int s = 0; s < subSteps; ++s) {
                m.step(volts[k], period / subSteps);
            }
            maxSpeed = std::max(maxSpeed, m.w);
        }
        const double loop = kp * model.kdc;
        const double predictedError = setpoint / (1.0 + loop);
        const double pole = a - kp * b;
        std::cout << std::format("{:>6.2f} {:>9.1f} {:>6.1f} {:>9.1f} {:>7.3f} {:>9.1f}   {:>6.2f} "
                                 "{:>6.2f} {:>6.2f} {:>6.2f}\n",
                                 kp, m.w, setpoint - m.w, predictedError, pole, maxSpeed,
                                 volts[396], volts[397], volts[398], volts[399]);
    }
    return 0;
}
