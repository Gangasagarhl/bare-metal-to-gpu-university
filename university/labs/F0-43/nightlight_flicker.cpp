// Forensic evidence: a night-light with ONE threshold, fed scripted dusk readings.
#include <iostream>

int main()
{
    const int darkBelow = 30;
    int minute = 0;
    int light = 0;
    int switches = 0;
    bool lampOn = false;

    while (std::cin >> minute >> light) {
        const bool wantOn = (light < darkBelow);
        if (wantOn != lampOn) {
            lampOn = wantOn;
            ++switches;
        }
        std::cout << "minute " << minute << "  light " << light << "  lamp "
                  << (lampOn ? "ON" : "off") << "\n";
    }
    std::cout << "lamp changed " << switches << " times\n";
    return 0;
}
