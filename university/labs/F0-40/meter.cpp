// Forensic evidence generator: pretend multimeter readings for a loop of
// battery -> switch -> resistor -> wire -> LED -> back to battery.
// Ideal model with PRETEND numbers: when no current flows, the whole battery
// voltage appears across the one place where the loop is broken or blocked.
#include <iostream>
#include <string>

int main()
{
    const double pretendForwardVolts = 2.0;
    std::string name;
    double battery = 0.0;
    double ohms = 0.0;
    std::string fault; // "none", "reversed" or "gap"

    while (std::cin >> name >> battery >> ohms >> fault) {
        double acrossSwitch = 0.0;
        double acrossResistor = 0.0;
        double acrossWire = 0.0;
        double acrossLed = 0.0;
        double milliamps = 0.0;
        if (fault == "none") {
            acrossLed = pretendForwardVolts;
            acrossResistor = battery - pretendForwardVolts;
            milliamps = acrossResistor * 1000.0 / ohms;
        } else if (fault == "reversed") {
            acrossLed = battery;
        } else {
            acrossWire = battery;
        }
        std::cout << "Circuit " << name << " (battery " << battery << " V, resistor "
                  << ohms << " ohms)\n"
                  << "  across switch:   " << acrossSwitch << " V\n"
                  << "  across resistor: " << acrossResistor << " V\n"
                  << "  across wire W2:  " << acrossWire << " V\n"
                  << "  across LED:      " << acrossLed << " V\n"
                  << "  current:         " << milliamps << " mA\n";
    }
    return 0;
}
