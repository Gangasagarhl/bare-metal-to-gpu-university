// F9-22 Listing 2: the open-loop step test, run on the simulated kit.
// Duty 0.25 from t = 0; the encoder is read every 10 ms. On the real kit the same
// program runs against your KitMotorIo; its output becomes identify.in.
#include "motor_io.hpp"
#include <format>
#include <iostream>

int main()
{
    SimulatedKit kit;
    const double period = 0.01;
    const double duty = 0.25;
    std::uint16_t before = kit.readEncoder();
    std::cout << std::format("duty {:.2f} period {:.3f} counts_per_rev {:.0f}\n", duty, period,
                             kit.countsPerRev);
    kit.setDuty(duty);
    for (int k = 1; k <= 100; ++k) {
        kit.advance(period);
        const std::uint16_t now = kit.readEncoder();
        std::cout << std::format("{:.2f} {:.1f}\n", k * period,
                                 speedFromCounts(now, before, kit.countsPerRev, period));
        before = now;
    }
    kit.setDuty(0.0);
    return 0;
}
