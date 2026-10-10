// F9-07 Listing 1: a speed limit and an emergency stop in the robot's loop.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const int speedLimit = 5;   // the fastest this robot is allowed to go (simulator units)
    // What the robot's plan asks for at each step:
    const std::vector<int> wanted = {2, 4, 9, 9, 9, 9, 9, 9, 9, 9};
    const int stopPressedAt = 6;   // in this test, someone presses the stop at step 6
    bool stopped = false;
    int position = 0;

    std::cout << "step  wanted  allowed  stop?  position\n";
    for (int step = 1; step <= 10; ++step) {
        if (step == stopPressedAt) {
            stopped = true;                        // the stop is latched: it stays on
        }
        int speed = std::min(wanted[step - 1], speedLimit);   // obey the speed limit
        if (stopped) {
            speed = 0;                             // the stop beats everything else
        }
        position = position + speed;
        std::cout << std::setw(4) << step << std::setw(8) << wanted[step - 1]
                  << std::setw(9) << speed << std::setw(7) << (stopped ? "yes" : "no")
                  << std::setw(10) << position << '\n';
    }
    return 0;
}
