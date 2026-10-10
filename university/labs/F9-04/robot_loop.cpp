// F9-04 Listing 1: the robot's brain is a loop: sense, think, act, again.
#include <iostream>

int main()
{
    const int goal = 6;        // a flag on the floor at square 6
    const int maxSteps = 20;   // safety limit: never loop more than this
    int position = 0;
    int step = 0;
    bool atGoal = false;

    while (!atGoal && step < maxSteps) {
        step = step + 1;
        atGoal = (position == goal);          // SENSE: am I standing on the flag?
        if (atGoal) {                         // THINK
            std::cout << "loop " << step << ": flag found at square " << position
                      << " -> stop\n";       // ACT
        } else {
            position = position + 1;          // ACT
            std::cout << "loop " << step << ": no flag -> forward to square "
                      << position << '\n';
        }
    }
    std::cout << "loop finished after " << step << " rounds\n";
    return 0;
}
