// Power in a resistor three ways: P = V x I = I x I x R = V x V / R.
// Each line of power.in: a name, the voltage across the resistor (V),
// its resistance (ohms) and the pretend power rating of the part (W).
// All numbers are exercise numbers, not values from a real datasheet.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    std::string name;
    double volts = 0.0;
    double ohms = 0.0;
    double ratingWatts = 0.0;

    std::cout << std::fixed << std::setprecision(4);
    while (std::cin >> name >> volts >> ohms >> ratingWatts) {
        const double amps = volts / ohms;
        const double pVI = volts * amps;
        const double pIIR = amps * amps * ohms;
        const double pVVR = volts * volts / ohms;
        std::cout << name << ": I = " << amps << " A, P = " << pVI << " W (V*I), " << pIIR
                  << " W (I*I*R), " << pVVR << " W (V*V/R)";
        if (pVI > ratingWatts) {
            std::cout << " -> ABOVE the pretend rating of " << ratingWatts << " W";
        } else {
            std::cout << " -> " << std::setprecision(1) << 100.0 * pVI / ratingWatts
                      << " % of the pretend rating" << std::setprecision(4);
        }
        std::cout << "\n";
    }
    return 0;
}
