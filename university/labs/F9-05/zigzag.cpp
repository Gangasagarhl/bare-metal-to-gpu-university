// F9-05 forensic evidence: the same robot with a correction that is too strong.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    const double gain = 1.8;    // how strongly the robot corrects (try other values!)
    double distance = 8.0;      // start 8 units to the right of the line

    std::cout << "step  distance      -10.......0.......+10\n";
    for (int step = 0; step <= 12; ++step) {
        // Draw one row of the text plot: '|' is the line, '*' is the robot.
        std::string row(21, ' ');
        row[10] = '|';
        long column = 10 + std::lround(distance);
        if (column >= 0 && column <= 20) {
            row[static_cast<std::size_t>(column)] = '*';
        }
        std::cout << std::setw(4) << step << std::setw(10) << std::fixed
                  << std::setprecision(2) << distance << "      " << row << '\n';

        double error = distance;            // SENSE: how far from the line am I?
        double steer = -gain * error;       // COMPARE and decide: steer back, sized by gain
        distance = distance + steer;        // CORRECT: the move changes the distance
    }
    return 0;
}
