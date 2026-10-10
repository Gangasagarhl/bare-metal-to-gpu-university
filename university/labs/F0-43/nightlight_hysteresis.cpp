// Answer-key check: the same readings with TWO thresholds (a "dead zone").
#include <iostream>

int main()
{
    const int turnOnBelow = 26;   // must get clearly dark to switch on
    const int turnOffAbove = 34;  // must get clearly bright to switch off
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
