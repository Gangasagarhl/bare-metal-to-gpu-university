// F9-01 Listing 1: a robot in a corridor that senses, thinks and acts.
#include <iostream>

int main()
{
    const int wall = 10;   // the wall stands at square 10 of the corridor
    int position = 0;      // the robot starts at square 0

    for (int step = 1; step <= 10; ++step) {
        int distance = wall - position;   // SENSE: how far away is the wall?
        bool tooClose = distance <= 2;    // THINK: is the wall close?
        if (tooClose) {
            std::cout << "step " << step << ": wall is " << distance
                      << " squares away -> stop\n";   // ACT: stay still
        } else {
            position = position + 1;                  // ACT: roll forward
            std::cout << "step " << step << ": wall is " << distance
                      << " squares away -> forward to square " << position << '\n';
        }
    }
    return 0;
}
