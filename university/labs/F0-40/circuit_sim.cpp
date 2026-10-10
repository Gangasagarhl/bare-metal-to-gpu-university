// A tiny circuit truth model: battery, switch, resistor and LED in one loop.
// The LED numbers are PRETEND exercise numbers, not from a real datasheet.
#include <iostream>
#include <string>

int main()
{
    const double pretendForwardVolts = 2.0;  // "pretend the datasheet says 2 V"
    const double pretendMaxMilliamps = 20.0; // "pretend the datasheet says 20 mA"

    std::string name;
    double batteryVolts = 0.0;
    int switchClosed = 0;
    int ledForward = 0;
    double resistorOhms = 0.0;

    while (std::cin >> name >> batteryVolts >> switchClosed >> ledForward >> resistorOhms) {
        std::cout << name << ": ";
        if (switchClosed == 0) {
            std::cout << "loop open at the switch -> no current, LED dark\n";
        } else if (ledForward == 0) {
            std::cout << "LED turned the wrong way -> it blocks, no current, LED dark\n";
        } else if (batteryVolts <= pretendForwardVolts) {
            std::cout << "battery too weak to push through the LED -> LED dark\n";
        } else if (resistorOhms <= 0.0) {
            std::cout << "NO RESISTOR -> nothing limits the current: too much! (never do this)\n";
        } else {
            const double milliamps = (batteryVolts - pretendForwardVolts) * 1000.0 / resistorOhms;
            std::cout << "current " << milliamps << " mA -> LED lit";
            if (milliamps > pretendMaxMilliamps) {
                std::cout << ", but ABOVE the pretend maximum of " << pretendMaxMilliamps
                          << " mA: choose a bigger resistor";
            }
            std::cout << "\n";
        }
    }
    return 0;
}
