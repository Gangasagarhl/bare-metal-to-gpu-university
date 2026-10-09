// Charge moved by a steady current: Q = I x t.
// The currents and times in charge.in are exercise numbers, not measurements.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    std::string name;
    double milliamps = 0.0;
    double seconds = 0.0;

    std::cout << std::fixed << std::setprecision(3);
    while (std::cin >> name >> milliamps >> seconds) {
        const double amps = milliamps / 1000.0;
        const double coulombs = amps * seconds;
        std::cout << name << ": " << milliamps << " mA for " << seconds << " s moves "
                  << coulombs << " C of charge\n";
    }
    return 0;
}
