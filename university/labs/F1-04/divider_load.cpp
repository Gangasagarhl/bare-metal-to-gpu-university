// Forensic evidence generator for "The divider that sags".
// A voltage divider (top resistor, bottom resistor) feeds a sensor input.
// divider_load.in holds the hidden load resistance of each plugged-in
// device (the answer key); "open" means nothing is plugged in.
// Ideal model with exercise numbers: 6 V supply, 10 kilo-ohm + 10 kilo-ohm.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double supply = 6.0;
    const double top = 10000.0;
    const double bottom = 10000.0;
    std::string device;
    std::string load;

    std::cout << std::fixed << std::setprecision(2);
    while (std::cin >> device >> load) {
        double lower = bottom;
        if (load != "open") {
            const double rLoad = std::stod(load);
            lower = 1.0 / (1.0 / bottom + 1.0 / rLoad);
        }
        const double out = supply * lower / (top + lower);
        const double fromSupplyMilliamps = supply / (top + lower) * 1000.0;
        std::cout << device << ": divider output " << out << " V, supply current "
                  << fromSupplyMilliamps << " mA\n";
    }
    return 0;
}
