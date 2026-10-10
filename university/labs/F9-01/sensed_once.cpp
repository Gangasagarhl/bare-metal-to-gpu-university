// F9-01 forensic evidence: the same robot, but with one line moved.
#include <iostream>

int main()
{
    const int wall = 10;   // the wall stands at square 10 of the corridor
    int position = 0;      // the robot starts at square 0

    int distance = wall - position;       // SENSE (now outside the loop)
    for (int step = 1; step <= 12; ++step) {
        bool tooClose = distance <= 2;    // THINK: is the wall close?
        if (tooClose) {
            std::cout << "step " << step << ": wall is " << distance
                      << " squares away -> stop\n";
        } else if (position + 1 >= wall) {
            std::cout << "step " << step << ": wall is " << distance
                      << " squares away -> forward... BUMP! hit the wall at square "
                      << wall << '\n';
        } else {
            position = position + 1;
            std::cout << "step " << step << ": wall is " << distance
                      << " squares away -> forward to square " << position << '\n';
        }
    }
    return 0;
}
