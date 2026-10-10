// Evidence for the KID103 final exam (forensic question): Priya's night-light.
// Priya says: "I used two thresholds like F0-43 and it flickers MORE than before,
// and the simulator says my LED is over the maximum although my sum said 20 mA."
// Pretend datasheet numbers (exercise values): LED forward 2 V, maximum 20 mA; battery 6 V.
#include <iostream>

int main()
{
    const double batteryVolts = 6.0;         // "pretend the battery pack gives 6 V"
    const double pretendForwardVolts = 2.0;  // "pretend the datasheet says 2 V"
    const double pretendMaxMilliamps = 20.0; // "pretend the datasheet says 20 mA"
    const double resistorOhms = 200.0;       // Priya's sum: (6 - 2) / 0.02 = 200

    const int turnOnBelow = 34;  // lamp on when it gets dark
    const int turnOffAbove = 26; // lamp off when it gets bright again

    const double milliamps = batteryVolts * 1000.0 / resistorOhms;
    std::cout << "pretend datasheet: forward voltage " << pretendForwardVolts << " V, maximum "
              << pretendMaxMilliamps << " mA; battery " << batteryVolts << " V; resistor "
              << resistorOhms << " ohms\n";
    std::cout << "LED current when lit: " << milliamps << " mA";
    if (milliamps > pretendMaxMilliamps) {
        std::cout << "  ABOVE the pretend maximum of " << pretendMaxMilliamps << " mA!";
    }
    std::cout << "\n";

    int minute = 0;
    int light = 0;
    int switches = 0;
    bool lampOn = false;
    while (std::cin >> minute >> light) {
        if (!lampOn && light < turnOnBelow) {
            lampOn = true;
            ++switches;
        } else if (lampOn && light > turnOffAbove) {
            lampOn = false;
            ++switches;
        }
        std::cout << "minute " << minute << "  light " << light << "  lamp "
                  << (lampOn ? "ON" : "off") << "\n";
    }
    std::cout << "lamp changed " << switches << " times\n";
    return 0;
}
