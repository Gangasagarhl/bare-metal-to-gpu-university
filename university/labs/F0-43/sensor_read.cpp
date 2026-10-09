// Reading three pretend sensors from a scripted table and making decisions.
// Each input line: time in seconds, light level (0-100, simulator units),
// distance in cm, tilted (0 or 1). All values are made up for this exercise.
#include <iostream>

int main()
{
    const int darkBelow = 30;      // our choice: light under 30 counts as "dark"
    const int nearBelowCm = 20;    // our choice: closer than 20 cm counts as "near"

    int seconds = 0;
    int light = 0;
    int distanceCm = 0;
    int tilted = 0;

    std::cout << "time light distance tilt | decisions\n";
    while (std::cin >> seconds >> light >> distanceCm >> tilted) {
        std::cout << seconds << " s  " << light << "   " << distanceCm << " cm   "
                  << tilted << "   | ";
        std::cout << (light < darkBelow ? "dark " : "bright ");
        std::cout << (distanceCm < nearBelowCm ? "NEAR " : "clear ");
        std::cout << (tilted == 1 ? "TILTED" : "level") << "\n";
    }
    return 0;
}
