// Evidence program for the forensic lab: the "all closed" light by the front door.
// Original rule: the green light is on when NOT (door open OR window open).
// Kofi "simplified" it before this run.
#include <iostream>

bool greenLightOriginal(bool doorOpen, bool windowOpen)
{
    return !(doorOpen || windowOpen);
}

bool greenLightSimplified(bool doorOpen, bool windowOpen)
{
    return !doorOpen || !windowOpen;
}

int main()
{
    std::cout << "door window | original simplified\n";
    for (bool door : {false, true}) {
        for (bool window : {false, true}) {
            std::cout << "  " << door << "     " << window << "    |    "
                      << greenLightOriginal(door, window) << "         "
                      << greenLightSimplified(door, window) << '\n';
        }
    }
    return 0;
}
