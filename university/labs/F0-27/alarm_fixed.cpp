// Fixed version for the forensic lab answer key.
// Original rule: the green light is on when NOT (door open OR window open).
// The simplified rule now follows De Morgan's law correctly.
#include <iostream>

bool greenLightOriginal(bool doorOpen, bool windowOpen)
{
    return !(doorOpen || windowOpen);
}

bool greenLightSimplified(bool doorOpen, bool windowOpen)
{
    return !doorOpen && !windowOpen;  // fixed: De Morgan turns OR into AND
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
