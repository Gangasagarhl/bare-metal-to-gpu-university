// F9-22 forensic evidence generator: "The motor that kicks every second or so".
// The speed loop of Listing 4 (same PI gains, pid.hpp) with a different speed
// calculation. Printed: every row where the measured speed is far from the true one,
// plus the two rows after it. The fault is described only in the answer key.
#include "../F9-21/pid.hpp"
#include "motor_io.hpp"
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    SimulatedKit kit;
    rb::PidConfig c;
    c.kp = 0.0482;
    c.ki = 0.2474;
    c.period = 0.01;
    c.outMin = -12.0;
    c.outMax = 12.0;
    rb::Pid pid = rb::Pid::create(c).value();
    std::uint16_t before = kit.readEncoder();
    int last = -10;
    std::cout << "  t (s)  counter  measured (rad/s)  duty   true speed (rad/s)\n";
    for (int k = 0; k < 500; ++k) {
        const std::uint16_t now = kit.readEncoder();
        const int delta = static_cast<int>(now) - static_cast<int>(before);
        before = now;
        const double measured = delta * 2.0 * 3.141592653589793 / kit.countsPerRev / c.period;
        const double duty = pid.update(300.0, measured) / kit.supplyVolts;
        const bool odd = k > 100 && std::abs(measured - kit.trueSpeed()) > 50.0;
        if (odd || k == last + 1 || k == last + 2) {
            if (odd && k != last + 1 && k != last + 2) {
                std::cout << "   ...\n";
            }
            std::cout << std::format("{:>7.2f} {:>8} {:>17.1f} {:>5.2f} {:>20.1f}\n", k * c.period,
                                     now, measured, duty, kit.trueSpeed());
            if (odd) {
                last = k;
            }
        }
        kit.setDuty(duty);
        kit.advance(c.period);
    }
    return 0;
}
