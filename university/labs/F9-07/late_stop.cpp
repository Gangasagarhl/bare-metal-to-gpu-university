// F9-07 forensic evidence: the stop button is only looked at every 4th step.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const int speedLimit = 5;
    const std::vector<int> wanted = {2, 4, 9, 9, 9, 9, 9, 9, 9, 9};
    const int stopPressedAt = 6;
    bool buttonDown = false;   // the real button
    bool stopped = false;      // what the program believes
    int position = 0;

    std::cout << "step  button  wanted  allowed  stopped?  position\n";
    for (int step = 1; step <= 10; ++step) {
        if (step >= stopPressedAt) {
            buttonDown = true;
        }
        if (step % 4 == 0 && buttonDown) {   // only checks the button every 4th step
            stopped = true;
        }
        int speed = std::min(wanted[step - 1], speedLimit);
        if (stopped) {
            speed = 0;
        }
        position = position + speed;
        std::cout << std::setw(4) << step << std::setw(8) << (buttonDown ? "down" : "up")
                  << std::setw(8) << wanted[step - 1] << std::setw(9) << speed
                  << std::setw(10) << (stopped ? "yes" : "no") << std::setw(10) << position
                  << '\n';
    }
    return 0;
}
