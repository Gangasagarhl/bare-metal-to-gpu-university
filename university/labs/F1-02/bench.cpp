// Forensic evidence generator: a test bench applies a voltage to each resistor
// of a batch and records the current. bench.in holds the TRUE resistance of
// each part (the answer key); the output is what the learner sees.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double appliedVolts = 5.0;
    std::string label;
    double trueOhms = 0.0;

    std::cout << "applied voltage: " << appliedVolts << " V\n";
    std::cout << std::fixed << std::setprecision(3);
    while (std::cin >> label >> trueOhms) {
        const double milliamps = appliedVolts / trueOhms * 1000.0;
        std::cout << label << "  current: " << std::setw(7) << milliamps << " mA\n";
    }
    return 0;
}
