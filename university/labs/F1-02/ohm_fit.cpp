// Ohm's law from a table of (voltage, current) pairs.
// For each pair: R = V / I. For the whole table: the best straight line
// through the origin, R = sum(V*I) / sum(I*I) (least squares).
// The pairs in ohm_fit.in are exercise numbers, not measurements.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    std::string part;
    double volts = 0.0;
    double milliamps = 0.0;
    double sumVI = 0.0;
    double sumII = 0.0;

    std::cout << std::fixed;
    while (std::cin >> part >> volts >> milliamps) {
        const double amps = milliamps / 1000.0;
        std::cout << part << ": V = " << std::setprecision(2) << volts << " V, I = " << milliamps
                  << " mA, V/I = " << std::setprecision(1) << volts / amps << " ohms\n";
        sumVI += volts * amps;
        sumII += amps * amps;
    }
    if (sumII > 0.0) {
        std::cout << "best-fit resistance through the origin: " << sumVI / sumII << " ohms\n";
    }
    return 0;
}
