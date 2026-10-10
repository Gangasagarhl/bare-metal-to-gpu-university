// Forensic evidence generator for "The meter that stopped reading current".
// Our own model meter (not a real product): voltage mode, and current mode
// with an internal fuse. Circuit: 6 V battery (pretend internal resistance
// 0.5 ohm) -> 1000 ohm resistor -> LED (pretend fixed 2 V) -> back.
// meter_log.in is the learner's notebook: step, mode and where the probes went.
// The pretend current range is 400 mA and the pretend fuse opens above 500 mA.
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double battery = 6.0;
    const double batteryInternal = 0.5;
    const double load = 1000.0;
    const double ledVolts = 2.0;
    const double meterShunt = 0.1;
    const double rangeMilliamps = 400.0;
    const double fuseMilliamps = 500.0;
    bool fuseOpen = false;

    int step = 0;
    std::string mode;
    std::string where;
    std::cout << std::fixed << std::setprecision(2);
    while (std::cin >> step >> mode >> where) {
        std::cout << "step " << step << "  mode " << mode << "  probes " << std::setw(16)
                  << std::left << where << std::right << "  display: ";
        const double loopMilliamps = (battery - ledVolts) / load * 1000.0;
        if (mode == "V" && where == "battery") {
            std::cout << battery - batteryInternal * loopMilliamps / 1000.0 << " V";
        } else if (mode == "V" && where == "resistor") {
            std::cout << loopMilliamps / 1000.0 * load << " V";
        } else if (mode == "A" && where == "in-series") {
            const double closed = (battery - ledVolts) / (load + meterShunt) * 1000.0;
            const double reading = fuseOpen ? 0.0 : closed;
            std::cout << reading << " mA";
        } else if (mode == "A" && where == "battery") {
            const double milliamps = battery / (batteryInternal + meterShunt) * 1000.0;
            if (fuseOpen) {
                std::cout << "0.00 mA";
            } else {
                std::cout << (milliamps > rangeMilliamps ? "OL" : "in range");
            }
            if (milliamps > fuseMilliamps) {
                fuseOpen = true;
            }
        } else {
            std::cout << "(not modelled)";
        }
        std::cout << "\n";
    }
    return 0;
}
