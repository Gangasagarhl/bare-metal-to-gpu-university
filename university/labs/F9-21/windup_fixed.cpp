// F9-21 Listing 3: the windup case of F9-18 (setpoint 0 -> 900 rad/s, PI gains
// 0.05 and 0.25, driver limit 12 V) run through pid.hpp, without and with anti-windup.
#include "pid.hpp"
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
    std::cout
        << "anti-windup   peak speed   overshoot   time above 909 rad/s   I term at t = 0.3 s\n";
    for (bool antiWindup : {false, true}) {
        rb::PidConfig c;
        c.kp = 0.05;
        c.ki = 0.25;
        c.period = 0.01;
        c.outMin = -12.0;
        c.outMax = 12.0;
        c.antiWindup = antiWindup;
        rb::Pid pid = rb::Pid::create(c).value();
        Motor m;
        double peak = 0.0;
        double above = 0.0;
        double iAt03 = 0.0;
        for (int k = 0; k < 300; ++k) {
            const double volts = pid.update(900.0, m.w);
            if (k == 30) {
                iAt03 = pid.integralTerm();
            }
            for (int s = 0; s < 100; ++s) {
                m.step(volts, 0.0001);
            }
            peak = std::max(peak, m.w);
            if (m.w > 909.0) {
                above += 0.01;
            }
        }
        std::cout << std::format("{:<11} {:>12.1f} {:>9.1f} % {:>20.2f} s {:>17.2f} V\n",
                                 antiWindup ? "on" : "off", peak,
                                 std::max(0.0, 100.0 * (peak - 900.0) / 900.0), above, iAt03);
    }
    return 0;
}
