// Forensic evidence generator for "The hot resistor".
// A board has three identical LED channels: supply -> resistor -> LED -> ground.
// hot_resistor.in holds the resistor value REALLY fitted in each channel
// (the answer key). The output is what a technician measured with a meter.
// Ideal model with PRETEND numbers: 9 V supply, LED forward voltage 2 V.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double supplyVolts = 9.0;
    const double pretendForwardVolts = 2.0;
    std::string channel;
    double fittedOhms = 0.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "supply: " << supplyVolts << " V\n";
    while (std::cin >> channel >> fittedOhms) {
        const double acrossResistor = supplyVolts - pretendForwardVolts;
        const double milliamps = acrossResistor / fittedOhms * 1000.0;
        std::cout << channel << ": across resistor " << acrossResistor << " V, across LED "
                  << pretendForwardVolts << " V, channel current " << milliamps << " mA\n";
    }
    return 0;
}
