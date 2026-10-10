// KID103 course project, reference solution (for markers): a night-light in the
// course's own text simulator. A light sensor, two thresholds (hysteresis) and an
// LED loop with PRETEND datasheet numbers (exercise values, not from a real part).
// Input: one line per minute "<minute> <light 0-100 simulator units>".
#include <iostream>

// Pretend datasheet numbers for the LED loop (F0-40): copy real ones from the kit's datasheets.
const double batteryVolts = 6.0;
const double pretendForwardVolts = 2.0;
const double pretendMaxMilliamps = 20.0;
const double resistorOhms = 400.0; // chosen below the pretend maximum on purpose

// The two thresholds (F0-43): a dead zone between them stops the flicker.
const int turnOnBelow = 26;
const int turnOffAbove = 34;

// Decide the lamp's next state from its current state and one reading.
bool decide(bool lampOn, int light)
{
    if (!lampOn && light < turnOnBelow) {
        return true;
    }
    if (lampOn && light > turnOffAbove) {
        return false;
    }
    return lampOn;
}

// Hand-worked cases for decide(): the dead zone must keep the old state.
int test_decide()
{
    int failures = 0;
    if (decide(false, 25) != true) { ++failures; }  // clearly dark: on
    if (decide(false, 30) != false) { ++failures; } // dead zone, was off: stays off
    if (decide(true, 30) != true) { ++failures; }   // dead zone, was on: stays on
    if (decide(true, 35) != false) { ++failures; }  // clearly bright: off
    if (decide(false, 26) != false) { ++failures; } // edge: 26 is not below 26
    std::cout << "test_decide: " << failures << " failures\n";
    return failures;
}

int main()
{
    if (test_decide() != 0) {
        return 1;
    }
    const double milliamps = (batteryVolts - pretendForwardVolts) * 1000.0 / resistorOhms;
    std::cout << "LED loop: " << batteryVolts << " V, " << resistorOhms << " ohms -> "
              << milliamps << " mA when lit (pretend maximum " << pretendMaxMilliamps << " mA)\n";
    if (milliamps > pretendMaxMilliamps) {
        std::cout << "STOP: above the pretend maximum, choose a bigger resistor\n";
        return 1;
    }

    int minute = 0;
    int light = 0;
    int switches = 0;
    int litMinutes = 0;
    bool lampOn = false;
    while (std::cin >> minute >> light) {
        const bool next = decide(lampOn, light);
        if (next != lampOn) {
            ++switches;
        }
        lampOn = next;
        if (lampOn) {
            ++litMinutes;
        }
        std::cout << "minute " << minute << "  light " << light << "  LED "
                  << (lampOn ? "LIT" : "dark") << "\n";
    }
    std::cout << "lamp changed " << switches << " times; LED lit for " << litMinutes
              << " minutes\n";
    return 0;
}
