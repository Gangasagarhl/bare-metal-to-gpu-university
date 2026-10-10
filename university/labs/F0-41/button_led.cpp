// A scripted push button, read once per tick, drives two lights:
// a "hold" light (on while pressed) and a "toggle" light (flips on each new press).
#include <iostream>

int main()
{
    int tick = 0;
    int pressed = 0;
    int wasPressed = 0;
    bool toggleLight = false;

    std::cout << "tick button hold-light toggle-light\n";
    while (std::cin >> pressed) {
        const bool holdLight = (pressed == 1);
        if (pressed == 1 && wasPressed == 0) { // a NEW press: up last tick, down now
            toggleLight = !toggleLight;
        }
        wasPressed = pressed;
        std::cout << tick << "    " << (pressed == 1 ? "down" : "up  ") << "   "
                  << (holdLight ? "ON " : "off") << "        "
                  << (toggleLight ? "ON" : "off") << "\n";
        ++tick;
    }
    return 0;
}
