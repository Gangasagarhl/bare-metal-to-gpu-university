// F9-16 Listing 1: open loop versus closed loop for the pretend motor.
// The motor (HW302 F1-69, reduced to first order in MA301 F0-68) is simulated in
// small steps; the controller runs every 10 ms, like a real control loop.
// Each line of the table is one "what if the world changes?" case.
#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <vector>

struct Motor
{
    // Pretend motor: R = 2 ohm, K = 0.01, J = 1e-5 kg m^2, b = 1e-6 N m s.
    double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6); // s
    double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);       // (rad/s) per V
    double kload = 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);      // (rad/s) per N m
    double w = 0.0;                                       // speed, rad/s

    void step(double volts, double loadTorque, double dt)
    {
        w += dt * (kdc * volts - kload * loadTorque - w) / tau;
    }
};

struct Case
{
    std::string name;
    double loadTorque;  // N m, a brake on the shaft
    double driverScale; // 1.0 = the driver delivers exactly the voltage asked for
    double kdcScale;    // 1.0 = this motor matches the model the designer used
};

double runToSteady(const Case& c, bool closedLoop)
{
    const double setpoint = 300.0; // rad/s
    const double kp = 0.2;         // V per (rad/s), closed loop only
    const double nominalKdc = Motor{}.kdc;
    const double controlPeriod = 0.01; // s
    const int subSteps = 100;
    Motor m;
    m.kdc *= c.kdcScale;
    for (int k = 0; k < 300; ++k) { // 3 s of control steps
        const double measured = m.w;
        double volts = closedLoop ? kp * (setpoint - measured) : setpoint / nominalKdc;
        volts = std::clamp(volts, -12.0, 12.0);
        for (int s = 0; s < subSteps; ++s) {
            m.step(volts * c.driverScale, c.loadTorque, controlPeriod / subSteps);
        }
    }
    return m.w;
}

int main()
{
    const std::vector<Case> cases = {
        {"nominal (model is exact)", 0.0, 1.0, 1.0},
        {"brake on the shaft 2 mN m", 0.002, 1.0, 1.0},
        {"battery sag: driver gives 85 %", 0.0, 0.85, 1.0},
        {"motor 10 % weaker than model", 0.0, 1.0, 0.9},
        {"all three together", 0.002, 0.85, 0.9},
    };
    std::cout << std::format(
        "model: tau = {:.4f} s, Kdc = {:.3f} rad/s per V, load gain = {:.0f} rad/s per N m\n",
        Motor{}.tau, Motor{}.kdc, Motor{}.kload);
    std::cout << std::format(
        "open-loop command = 300 / Kdc = {:.4f} V; closed loop: V = 0.2 * (300 - speed)\n",
        300.0 / Motor{}.kdc);
    std::cout
        << "case                              open loop   closed loop   (setpoint 300 rad/s)\n";
    for (const Case& c : cases) {
        std::cout << std::format("{:<32} {:>10.1f} {:>13.1f}\n", c.name, runToSteady(c, false),
                                 runToSteady(c, true));
    }
    return 0;
}
