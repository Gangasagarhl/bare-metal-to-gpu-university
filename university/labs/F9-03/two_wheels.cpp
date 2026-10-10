// F9-03 Listing 1: two wheel motors and their turn counters (encoders).
#include <iostream>

// Drive for 5 steps. Each step, each wheel turns the number of times asked.
void drive(int leftTurnsPerStep, int rightTurnsPerStep)
{
    const int unitsPerTurn = 10;   // our simulated wheel rolls 10 units per turn
    int leftCount = 0;             // left encoder: counts the left wheel's turns
    int rightCount = 0;            // right encoder: counts the right wheel's turns

    for (int step = 1; step <= 5; ++step) {
        leftCount = leftCount + leftTurnsPerStep;
        rightCount = rightCount + rightTurnsPerStep;
    }
    int leftDistance = leftCount * unitsPerTurn;
    int rightDistance = rightCount * unitsPerTurn;

    std::cout << "left " << leftTurnsPerStep << ", right " << rightTurnsPerStep
              << " per step: counts " << leftCount << " and " << rightCount
              << ", wheels rolled " << leftDistance << " and " << rightDistance
              << " units -> ";
    if (leftDistance == 0 && rightDistance == 0) {
        std::cout << "stands still\n";
    } else if (leftDistance == rightDistance) {
        std::cout << "straight line\n";
    } else if (leftDistance > rightDistance) {
        std::cout << "curves to the right\n";
    } else {
        std::cout << "curves to the left\n";
    }
}

int main()
{
    drive(4, 4);
    drive(4, 2);
    drive(2, 4);
    drive(0, 0);
    return 0;
}
