// F9-17 Listing 2: the first 0.25 s of three step responses (setpoint 0 -> 300 rad/s),
// one row per control period of 10 ms. Used for Figure 2.
#include <algorithm>
#include <array>
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
    const std::array<double, 3> gains = {0.02, 0.1, 0.3};
    std::array<Motor, 3> motors;
    std::cout << "  t (s)   Kp=0.02   Kp=0.1   Kp=0.3   (speed in rad/s, setpoint 300)\n";
    for (int k = 0; k <= 25; ++k) {
        std::cout << std::format("{:>7.2f} {:>9.1f} {:>8.1f} {:>8.1f}\n", k * 0.01, motors[0].w,
                                 motors[1].w, motors[2].w);
        for (int g = 0; g < 3; ++g) {
            const double volts = std::clamp(gains[g] * (300.0 - motors[g].w), -12.0, 12.0);
            for (int s = 0; s < 100; ++s) {
                motors[g].step(volts, 0.0001);
            }
        }
    }
    return 0;
}
