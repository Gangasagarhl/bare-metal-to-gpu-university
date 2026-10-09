// Forensic evidence: the same toggle light, written WITHOUT remembering the last tick.
#include <iostream>

int main()
{
    int tick = 0;
    int pressed = 0;
    bool toggleLight = false;

    std::cout << "tick button toggle-light\n";
    while (std::cin >> pressed) {
        if (pressed == 1) {
            toggleLight = !toggleLight;
        }
        std::cout << tick << "    " << (pressed == 1 ? "down" : "up  ") << "   "
                  << (toggleLight ? "ON" : "off") << "\n";
        ++tick;
    }
    return 0;
}
