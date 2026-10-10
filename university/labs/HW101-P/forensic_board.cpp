// Forensic evidence generator for the HW101 final exam question "The board with two strangers".
// A four-channel indicator board: supply -> resistor marked 1 kilo-ohm on the schematic -> LED -> ground,
// the same channel model as F1-03's hot_resistor.cpp. forensic_board.in holds the resistor value
// REALLY fitted in each channel (the answer key, not shown to candidates). The output is what the
// technician read with the model meter. Ideal model with PRETEND numbers: 9 V supply, LED 2 V.
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
    std::cout << "board B-7, supply: " << supplyVolts << " V, schematic: every channel 1 kilo-ohm, LED, ground\n";
    while (std::cin >> channel >> fittedOhms) {
        const double acrossResistor = supplyVolts - pretendForwardVolts;
        const double milliamps = acrossResistor / fittedOhms * 1000.0;
        std::cout << channel << ": across resistor " << acrossResistor << " V, across LED "
                  << pretendForwardVolts << " V, channel current " << std::setw(6) << milliamps << " mA\n";
    }
    return 0;
}
