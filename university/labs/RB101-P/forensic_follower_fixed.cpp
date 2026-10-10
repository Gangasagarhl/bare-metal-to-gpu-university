// Answer key for the RB101 final exam (forensic question): Priya's line follower, fixed.
// Two lines changed: the stop is latched (set once, never cleared), and the robot moves by
// the ALLOWED command (after the speed limit and the stop), not by the wished steer.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double gain = 0.5;         // how strongly the robot corrects (F9-05)
    const double speedLimit = 3.0;   // the biggest sideways move allowed per step (F9-07)
    const int stopPressedAt = 2;     // in this test, someone presses the stop at step 2
    double distance = 8.0;           // start 8 units to the right of the line
    bool stopped = false;

    std::cout << "step  distance    steer  allowed  stop?      -10.......0.......+10\n";
    for (int step = 0; step <= 12; ++step) {
        if (step == stopPressedAt) {
            stopped = true;                                // latched: nothing clears it
        }

        double error = distance;                           // SENSE
        double steer = -gain * error;                      // COMPARE: how far to move back
        double allowed = std::clamp(steer, -speedLimit, speedLimit);   // obey the speed limit
        if (stopped) {
            allowed = 0.0;                                 // the stop beats everything else
        }

        std::string row(21, ' ');
        row[10] = '|';
        long column = 10 + std::lround(distance);
        if (column >= 0 && column <= 20) {
            row[static_cast<std::size_t>(column)] = '*';
        }
        std::cout << std::setw(4) << step << std::fixed << std::setprecision(2)
                  << std::setw(10) << distance << std::setw(9) << steer << std::setw(9)
                  << allowed << std::setw(7) << (stopped ? "yes" : "no") << "      " << row
                  << '\n';

        distance = distance + allowed;                     // CORRECT: move by the allowed command
    }
    return 0;
}
