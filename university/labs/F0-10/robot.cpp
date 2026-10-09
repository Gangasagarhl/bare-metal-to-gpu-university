#include "robot_world.hpp"

int main()
{
    Robot robot;
    for (int tick = 1; tick <= 12 && !robot.atGoal(); ++tick) {
        const bool wall = robot.sensorWallAhead();   // 1. sense
        if (wall) {                                  // 2. think
            robot.motorTurnRight();                  // 3. act
            robot.report(tick, "wall ahead, turn right");
        } else {
            robot.motorForward();
            robot.report(tick, "path clear, forward");
        }
    }
    std::cout << (robot.atGoal() ? "goal reached" : "goal not reached") << ", bumps: "
              << robot.bumps << '\n';
    return 0;
}
