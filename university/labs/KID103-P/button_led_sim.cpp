// KID103 practical: a button-controlled LED in the course's own text simulator.
// Loop: battery -> push button -> resistor -> LED -> back to the battery.
// The LED numbers are PRETEND exercise numbers, not from a real datasheet.
// Input: first line "<battery volts> <LED forward 1/0> <resistor ohms>",
// then one button reading per tick (1 = pressed, 0 = released) until the end.
#include <iostream>

int main()
{
    const double pretendForwardVolts = 2.0;  // "pretend the datasheet says 2 V"
    const double pretendMaxMilliamps = 20.0; // "pretend the datasheet says 20 mA"

    double batteryVolts = 0.0;
    int ledForward = 0;
    double resistorOhms = 0.0;
    if (!(std::cin >> batteryVolts >> ledForward >> resistorOhms)) {
        std::cout << "no loop line: nothing to simulate\n";
        return 1;
    }
    std::cout << "loop: battery " << batteryVolts << " V, LED "
              << (ledForward == 1 ? "forward" : "REVERSED") << ", resistor " << resistorOhms
              << " ohms\n";
    std::cout << "pretend datasheet: forward voltage " << pretendForwardVolts << " V, maximum "
              << pretendMaxMilliamps << " mA\n";

    // Check the loop before any tick: a real build would be checked by the adult first.
    bool loopOk = true;
    double milliamps = 0.0;
    if (ledForward != 1) {
        std::cout << "LED reversed: it blocks, so the LED stays dark whatever the button does\n";
        loopOk = false;
    } else if (batteryVolts <= pretendForwardVolts) {
        std::cout << "battery too weak to push through the LED: LED stays dark\n";
        loopOk = false;
    } else if (resistorOhms <= 0.0) {
        std::cout << "NO RESISTOR: nothing limits the current (never do this): not simulated\n";
        loopOk = false;
    } else {
        milliamps = (batteryVolts - pretendForwardVolts) * 1000.0 / resistorOhms;
        std::cout << "current when the button is pressed: " << milliamps
                  << " mA (pretend maximum " << pretendMaxMilliamps << " mA)\n";
        if (milliamps > pretendMaxMilliamps) {
            std::cout << "ABOVE the pretend maximum: choose a bigger resistor: not simulated\n";
            loopOk = false;
        }
    }

    int tick = 0;
    int pressed = 0;
    int litTicks = 0;
    std::cout << "tick button LED\n";
    while (std::cin >> pressed) {
        const bool lit = loopOk && (pressed == 1); // the button is the switch of the loop
        if (lit) {
            ++litTicks;
        }
        std::cout << tick << "    " << (pressed == 1 ? "down" : "up  ") << "   "
                  << (lit ? "LIT" : "dark") << "\n";
        ++tick;
    }
    std::cout << "LED was lit for " << litTicks << " of " << tick << " ticks\n";
    return 0;
}
