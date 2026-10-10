// KID103 practical: STARTING FILE. A button-controlled LED in the course's own
// text simulator. Loop: battery -> push button -> resistor -> LED -> back.
// The LED numbers are PRETEND exercise numbers, not from a real datasheet.
// Input: first line "<battery volts> <LED forward 1/0> <resistor ohms>",
// then one button reading per tick (1 = pressed, 0 = released) until the end.
//
// This file builds and runs, but it is WRONG in five places marked TODO.
// Finish it so that it follows the specification in the exam paper.
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

    bool loopOk = true;
    // TODO 1: if the LED is reversed, print the "LED reversed" line and set loopOk to false.
    // TODO 2: if the battery is not more than the forward voltage, print the
    //         "battery too weak" line and set loopOk to false.
    // TODO 3: if there is no resistor (0 ohms), print the "NO RESISTOR" line and set loopOk to false.

    // TODO 4: the resistor's share of the push is battery - forward voltage (F0-40), and the
    //         current must be compared with the pretend maximum (loopOk = false when above).
    const double milliamps = batteryVolts * 1000.0 / resistorOhms;
    std::cout << "current when the button is pressed: " << milliamps
              << " mA (pretend maximum " << pretendMaxMilliamps << " mA)\n";

    int tick = 0;
    int pressed = 0;
    int litTicks = 0;
    std::cout << "tick button LED\n";
    while (std::cin >> pressed) {
        // TODO 5: the LED is lit only while the button is pressed AND the loop is ok;
        //         count the lit ticks.
        const bool lit = loopOk;
        std::cout << tick << "    " << (pressed == 1 ? "down" : "up  ") << "   "
                  << (lit ? "LIT" : "dark") << "\n";
        ++tick;
    }
    std::cout << "LED was lit for " << litTicks << " of " << tick << " ticks\n";
    return 0;
}
