// RB101 course project (reference solution): a simulated line follower with a gentle gain,
// a speed limit and a latched emergency stop, tested on a scripted run with "pushes".
// Input: line 1 = the step at which the stop is pressed (0 = never); then one push per step
// (a sideways nudge in simulator units, like a bend in the line or a bump from the floor).
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

const double gain = 0.5;          // gentle: below 1, so the robot never crosses the line by itself
const double speedLimit = 3.0;    // the biggest sideways move allowed per step (simulator units)

// THINK: the whole steering rule in one place. Returns the allowed sideways move.
double decide(double distance, bool stopped)
{
    double error = distance;                                   // target is 0 (on the line)
    double steer = 0.0 - gain * error;                         // proportional correction
    double allowed = std::clamp(steer, -speedLimit, speedLimit);   // speed limit on every command
    if (stopped) {
        allowed = 0.0;                                         // the latched stop beats everything
    }
    return allowed;
}

// Hand-worked check of the rule (done before any run): four cases from the plan on paper.
int selfCheck()
{
    int failures = 0;
    auto expect = [&](double distance, bool stopped, double wanted, const char* why) {
        double got = decide(distance, stopped);
        bool ok = std::fabs(got - wanted) < 0.0001;
        std::cout << "  " << (ok ? "ok  " : "FAIL") << "  distance " << std::setw(5) << distance
                  << (stopped ? "  stop pressed" : "  stop released") << "  -> move "
                  << std::setw(5) << got << "  (" << why << ")\n";
        if (!ok) { failures = failures + 1; }
    };
    std::cout << std::fixed << std::setprecision(2) << "self-check of the rule:\n";
    expect(8.0, false, -3.0, "far right: -0.5 x 8 = -4, limited to -3");
    expect(-2.0, false, 1.0, "a little left: -0.5 x -2 = +1");
    expect(0.0, false, 0.0, "on the line: no correction");
    expect(-8.0, true, 0.0, "stop pressed: no move whatever the error");
    std::cout << "self-check: " << failures << " failures\n\n";
    return failures;
}

int main()
{
    if (selfCheck() != 0) {
        std::cout << "the rule is wrong: not running the robot\n";
        return 1;
    }
    int stopPressedAt = 0;
    if (!(std::cin >> stopPressedAt)) {
        std::cout << "input needed: stop step, then one push per step\n";
        return 1;
    }
    double distance = 8.0;        // start 8 units to the right of the line
    bool stopped = false;
    int crossings = 0;
    int step = 0;
    double push = 0.0;

    std::cout << "step  distance   push  allowed  stop?      -10.......0.......+10\n";
    while (std::cin >> push) {
        step = step + 1;
        if (step == stopPressedAt) {
            stopped = true;                                    // latched: nothing clears it
        }
        double allowed = decide(distance, stopped);            // SENSE + THINK
        std::string row(21, ' ');
        row[10] = '|';
        long column = 10 + std::lround(distance);
        if (column >= 0 && column <= 20) {
            row[static_cast<std::size_t>(column)] = '*';
        }
        std::cout << std::setw(4) << step << std::setw(10) << distance << std::setw(7) << push
                  << std::setw(9) << allowed << std::setw(7) << (stopped ? "yes" : "no")
                  << "      " << row << '\n';
        double next = distance + allowed + push;               // ACT: the move, plus the push
        if ((distance > 0 && next < 0) || (distance < 0 && next > 0)) {
            crossings = crossings + 1;
        }
        distance = next;
    }
    std::cout << "after " << step << " steps: distance " << distance << ", crossed the line "
              << crossings << " times, stop " << (stopped ? "pressed" : "never pressed") << '\n';
    return 0;
}
